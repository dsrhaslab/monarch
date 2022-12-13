//
// Created by dantas on 19/10/20.
//

#include <iostream>
#include <cstring>

#include "../../utils/logging/singleton_logger.h"
#include "hierarchical_stage.h"
#include "hierarchical_stage_builder.h"
#include "storage_drivers/states/storage_driver_blocking_state.h"
#include "storage_drivers/states/storage_driver_eventual_state.h"

HierarchicalStageBuilder* HierarchicalStage::create(int hierarchy_size){
    return new HierarchicalStageBuilder{hierarchy_size};
};

HierarchicalStage::HierarchicalStage(int hierarchy_size) {
    n_used_threads = 0;
    shared_thread_pool = false;
    storage_hierarchy_size = hierarchy_size;
    source_level = hierarchy_size - 1;
    private_debug_enabled = true;
    debug_logger = SingletonLogger::get_instance();
}

HierarchicalStage::HierarchicalStage(HierarchicalStage* hs){
    n_used_threads = hs->n_used_threads;
    shared_thread_pool = hs->shared_thread_pool;
    storage_hierarchy= hs->storage_hierarchy;
    storage_hierarchy_size = hs->storage_hierarchy_size;
    storage_hierarchy_thread_pools = hs->storage_hierarchy_thread_pools;
    source_level = hs->source_level;
    private_debug_enabled = hs->private_debug_enabled;
    debug_logger = hs->debug_logger;
}

void HierarchicalStage::debug_write(const std::string& msg){
    debug_logger->_write("[HierarchicalStage] " + msg);
}

bool HierarchicalStage::debug_is_activated(){
    return debug_logger->is_activated() && private_debug_enabled;
}

bool HierarchicalStage::has_staging_levels(){
    return storage_hierarchy_size > 1;
}

bool HierarchicalStage::type_is(StorageDriverType type, int level){
    return storage_hierarchy[level]->get_type() == type;
}

bool HierarchicalStage::subtype_is(StorageDriverSubType subtype, int level){
    return storage_hierarchy[level]->get_subtype() == subtype;
}

bool HierarchicalStage::driver_state_type_is(StorageDriverStateType state_type, int level){
    if(storage_hierarchy[level]->state == nullptr){
        return state_type == StorageDriverStateType::NONE;
    }
    return storage_hierarchy[level]->state->get_type() == state_type;
}

StorageDriver* HierarchicalStage::get_driver(int level){
    return storage_hierarchy[level];
}

FileSystemDriver* HierarchicalStage::get_source_driver(){
    return static_cast<FileSystemDriver*>(get_driver(source_level));
}

FileSystemDriver* HierarchicalStage::get_file_system_driver(int level){
    return static_cast<FileSystemDriver*>(storage_hierarchy[level]);
}

PosixFileSystemDriver* HierarchicalStage::get_posix_file_system_driver(int level){
    return static_cast<PosixFileSystemDriver*>(storage_hierarchy[level]);
}

MemoryBufferDriver* HierarchicalStage::get_memory_buffer_driver(int level){
    return static_cast<MemoryBufferDriver*>(storage_hierarchy[level]);
}

void HierarchicalStage::make_blocking_driver_wrappers(){
    for(int i = 0; i < source_level; i++){
        if(driver_state_type_is(StorageDriverStateType::ALLOCABLE, i)){
            if(debug_is_activated()) {
                debug_write("Making storage level " + std::to_string(i) + " wrapper type blocking");
            }
            get_driver(i)->state = new StorageDriverBlockingState(static_cast<StorageDriverAllocableState*>(get_driver(i)->state));
        }
    }
}

void HierarchicalStage::make_eventual_driver_wrappers(){
    for(int i = 0; i < source_level; i++){
        if(driver_state_type_is(StorageDriverStateType::ALLOCABLE, i)){
            if(debug_is_activated()) {
                debug_write("Making storage level " + std::to_string(i) + " wrapper type eventual");
            }
            get_driver(i)->state = new StorageDriverEventualState(static_cast<StorageDriverAllocableState*>(get_driver(i)->state));
        }
    }
}

int HierarchicalStage::find_free_level(Info* fi, int offset){
    for(int i = offset; i < source_level; i++) {
        if (!get_driver(i)->state->becomes_full(fi)) {
            return i;
        }
    }
    return -1;
}

int HierarchicalStage::find_free_level(Info* fi) {
    return find_free_level(fi, 0);
}

StorageDriver* HierarchicalStage::find_free_storage_driver(Info* fi, int offset){
    for(int i = offset; i < source_level; i++) {
        auto driver = get_driver(i);
        if (!driver->state->becomes_full(fi)) {
            return driver;
        }
    }
    return nullptr;
}

StorageDriver* HierarchicalStage::find_free_storage_driver(Info* i){
    return find_free_storage_driver(i,0);
}

//TODO more generic
int HierarchicalStage::alloc_free_level(Info* fi, int offset){
    for(int i = offset; i < source_level; i++) {
        if (get_driver(i)->state->allocate_storage(fi).state != STORAGE_FULL) {
            return i;
        }
    }
    return -1;
}

//TODO check this out. I don't remeber this logic
int HierarchicalStage::eventual_free_level(Info* fi, int offset){
    for(int i = offset; i < source_level; i++) {
        if (!get_driver(i)->state->becomes_full(fi)) {
            return i;
        }
    }
    return -1;
}

void HierarchicalStage::apply_to_fs_drivers(std::function<void(FileSystemDriver*)> func){
    for(int i = 0; i < storage_hierarchy_size; i++){
        if(type_is(StorageDriverType::FILE_SYSTEM, i)){
            func(get_file_system_driver(i));
        }
    }
}

size_t HierarchicalStage::get_capacity(int level){
    if (!driver_state_type_is(StorageDriverStateType::NONE, level)){
        return get_driver(level)->state->get_max_storage_size();
    }
    return 0;
}

size_t HierarchicalStage::get_full_capacity(){
    size_t full_capacity = 0;
    for (int i = 0; i < source_level; i++) {
        full_capacity += get_capacity(i);
    }
    return full_capacity;
}

void HierarchicalStage::init(std::vector<std::string>& dirs){
    //ignore source level
    for(int i = 0; i < source_level; i++){
        if(type_is(StorageDriverType::FILE_SYSTEM, i)){
            get_file_system_driver(i)->create_environment(dirs, true);
        }
    }
    //TODO this assumes last is always fs. Make it future proof!
    get_file_system_driver(source_level)->create_environment(dirs, false);

    if(debug_logger->is_activated())
        debug_write("Created storage environment");
}

int HierarchicalStage::get_source_level(){
    return source_level;
}

ctpl::thread_pool* HierarchicalStage::t_pool(int storage_index){
    if (!shared_thread_pool)
        return storage_hierarchy_thread_pools[storage_index];
    return storage_hierarchy_thread_pools[0];
}

//TODO use decent method -> print to file in home_dir
std::vector<std::string> HierarchicalStage::configs() {
    std::vector<std::string> res;
    res.push_back("- HierarchicalModule\n");
    res.push_back("\t- hierarchy " + std::to_string(storage_hierarchy_size) + "\n");
    res.push_back("\t\thierarchy_size: " + std::to_string(storage_hierarchy_size) + "\n");
    std::string b = shared_thread_pool ? "true" : "false";
    res.push_back("\t\tshared_pool: " + b + "\n");
    res.push_back("\t\ttotal_used_threads: " + std::to_string(n_used_threads) + "\n");

    for(int i = 0; i < storage_hierarchy_size; i++){
        res.push_back("\t\t- storage_driver_" + std::to_string(i) + ":\n");
        if(!shared_thread_pool){
            res.push_back("\t\t\tthread_pool_size: " + std::to_string(storage_hierarchy_thread_pools[i]->size()) + "\n");
        }

        auto* driver = storage_hierarchy[i];
        for(const auto& str : driver->configs())
            res.push_back("\t\t\t" + str);
    }
    return res;
}

void HierarchicalStage::print(){
    for (const auto &str : configs())
        std::cout << str;
}