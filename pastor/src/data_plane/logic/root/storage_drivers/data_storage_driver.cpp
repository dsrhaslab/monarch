//
// Created by dantas on 06/11/20.
//

#include "data_storage_driver.h"
#include "data_storage_driver_builder.h"
#include <iostream>

DataStorageDriverBuilder DataStorageDriver::create(DriverType type){
    return DataStorageDriverBuilder{type};
}

bool BaseDataStorageDriver::alloc_type(){
    return false;
}

std::vector<std::string> BaseDataStorageDriver::configs(){
    std::vector<std::string> configs;
    configs.push_back("block_size: " + std::to_string(block_size) + "\n");
    configs.push_back("storage_prefix: " + storage_prefix + "\n");
    return configs;
}

std::string BaseDataStorageDriver::prefix(){
    return storage_prefix;
}

AllocableDataStorageDriver::AllocableDataStorageDriver(BaseDataStorageDriver* base_storage_driver){
    AllocableDataStorageDriver::base_storage_driver = base_storage_driver;
}

AllocableDataStorageDriver::AllocableDataStorageDriver(AllocableDataStorageDriver* to_copy){
    AllocableDataStorageDriver::base_storage_driver = to_copy->base_storage_driver;
    AllocableDataStorageDriver::current_size = to_copy->current_size;
    AllocableDataStorageDriver::max_storage_size = to_copy->max_storage_size;
    AllocableDataStorageDriver::max_storage_occupation_threshold = to_copy->max_storage_occupation_threshold;
    delete to_copy;
}

Status<ssize_t> AllocableDataStorageDriver::allocate_storage(FileInfo* fi){
    ssize_t bytes_to_alloc = base_storage_driver->sizeof_content(fi);
    if (bytes_to_alloc + current_size > max_storage_size)
        return {STORAGE_FULL};

    current_size += bytes_to_alloc;
    float current_occupation = current_size / max_storage_size;

    if (current_occupation >= max_storage_occupation_threshold)
        return {SUCCESS, OCCUPATION_THRESHOLD_REACHED, bytes_to_alloc};
    return {bytes_to_alloc};
}

Status<ssize_t> AllocableDataStorageDriver::conditional_allocate_storage(FileInfo* fi){
    ssize_t bytes_to_alloc = base_storage_driver->sizeof_content(fi);
    if(bytes_to_alloc + current_size <= max_storage_size){
        current_size += bytes_to_alloc;
        return {bytes_to_alloc};
    }else{
        return {STORAGE_FULL};
    }
}

Status<ssize_t> AllocableDataStorageDriver::free_storage(FileInfo* fi){
    ssize_t bytes_to_alloc = base_storage_driver->sizeof_content(fi);
    current_size -= bytes_to_alloc;
    return {bytes_to_alloc};
}

size_t AllocableDataStorageDriver::resize(size_t new_size){
    max_storage_size = new_size;
    return new_size;
}

bool AllocableDataStorageDriver::becomesFull(FileInfo* fi){
    return base_storage_driver->sizeof_content(fi) + current_size >= max_storage_size;
}

bool AllocableDataStorageDriver::occupation_threshold_reached() const{
    return current_size >= max_storage_size * max_storage_occupation_threshold;
}

size_t AllocableDataStorageDriver::current_storage_size(){
    return current_size;
}

size_t AllocableDataStorageDriver::get_max_storage_size() const{
    return max_storage_size;
}

Status<ssize_t> AllocableDataStorageDriver::read(File* f){
    return base_storage_driver->read(f);
}

Status<ssize_t> AllocableDataStorageDriver::read(FileInfo* fi, char* result, uint64_t offset, size_t n, bool _64_option){
    return base_storage_driver->read(fi, result, offset, n, _64_option);
}

Status<ssize_t> AllocableDataStorageDriver::write(File* f){
    return base_storage_driver->write(f);
}

Status<ssize_t> AllocableDataStorageDriver::remove(FileInfo* fi){
    return base_storage_driver->remove(fi);
}

File* AllocableDataStorageDriver::remove_for_copy(FileInfo* fi){
    return base_storage_driver->remove_for_copy(fi);
}

ssize_t AllocableDataStorageDriver::sizeof_content(FileInfo* fi){
    return base_storage_driver->sizeof_content(fi);
}

bool AllocableDataStorageDriver::in_memory_type(){
    return base_storage_driver->in_memory_type();
}

bool AllocableDataStorageDriver::file_system_type(){
    return base_storage_driver->file_system_type();
}

void AllocableDataStorageDriver::create_environment(std::vector<std::string>& dirs, bool enable_write){
    base_storage_driver->create_environment(dirs, enable_write);
}

std::vector<std::string> AllocableDataStorageDriver::configs(){
    auto configs = base_storage_driver->configs();
    configs.push_back("max_storage_size: " + std::to_string(max_storage_size) + "\n");
    configs.push_back("max_storage_occupation_threshold: " + std::to_string(max_storage_occupation_threshold) + "\n");
    return configs;
}

std::string AllocableDataStorageDriver::prefix() {
    return base_storage_driver->prefix();
}

BaseDataStorageDriver* AllocableDataStorageDriver::get_base_storage_driver(){
    return base_storage_driver;
}

BlockingAllocableDataStorageDriver::BlockingAllocableDataStorageDriver(BaseDataStorageDriver *baseStorageDriver)
        : AllocableDataStorageDriver(baseStorageDriver) {

    mutex = std::make_unique<std::mutex>();
    is_full = std::make_unique<std::condition_variable>();
    waiting = 0;
}

BlockingAllocableDataStorageDriver::BlockingAllocableDataStorageDriver(AllocableDataStorageDriver* base) : AllocableDataStorageDriver(base){
    mutex = std::make_unique<std::mutex>();
    is_full = std::make_unique<std::condition_variable>();
    waiting = 0;
}

const std::unique_ptr<std::mutex> &BlockingAllocableDataStorageDriver::get_mutex() const{
    return mutex;
}

const std::unique_ptr<std::condition_variable> &BlockingAllocableDataStorageDriver::get_cond_var() const{
    return is_full;
}

bool BlockingAllocableDataStorageDriver::blocking_type() {
    return true;
}

Status<ssize_t> BlockingAllocableDataStorageDriver::allocate_storage(FileInfo* fi){
    ssize_t bytes_to_alloc = base_storage_driver->sizeof_content(fi);
    std::unique_lock<std::mutex> ul(*mutex);
    bool waited = false;
    while(bytes_to_alloc + current_size > max_storage_size) {
        //for spurious wakeup
        if(!waited)
            waiting++;
        waited = true;
        is_full->wait(ul);
    }
    if(waited)
        waiting--;
    current_size += bytes_to_alloc;
    return {bytes_to_alloc};
}

Status<ssize_t> BlockingAllocableDataStorageDriver::conditional_allocate_storage(FileInfo* fi){
    ssize_t bytes_to_alloc = base_storage_driver->sizeof_content(fi);
    if(bytes_to_alloc + current_size <= max_storage_size){
        current_size += bytes_to_alloc;
        return {bytes_to_alloc};
    }else{
        return {STORAGE_FULL};
    }
}

Status<ssize_t> BlockingAllocableDataStorageDriver::free_storage(FileInfo* fi){
    ssize_t bytes_to_alloc = base_storage_driver->sizeof_content(fi);
    std::unique_lock<std::mutex> ul(*mutex);
    current_size -= bytes_to_alloc;
    bool notify = waiting > 0;
    ul.unlock();
    if(notify)
        is_full->notify_all();
    return {bytes_to_alloc};
}

size_t BlockingAllocableDataStorageDriver::resize(size_t new_size){
    std::unique_lock<std::mutex> ul(*mutex);
    max_storage_size = new_size;
    return new_size;
}

size_t BlockingAllocableDataStorageDriver::current_storage_size(){
    std::unique_lock<std::mutex> ul(*mutex);
    return current_size;
}

bool BlockingAllocableDataStorageDriver::becomesFull(FileInfo* fi){
    std::unique_lock<std::mutex> ul(*mutex);
    return base_storage_driver->sizeof_content(fi) + current_size >= max_storage_size;
}

std::vector<std::string> BlockingAllocableDataStorageDriver::configs() {
    auto configs = AllocableDataStorageDriver::configs();
    configs.push_back("Blocking: true\n");
    return configs;
}

bool EventualAllocableDataStorageDriver::eventual_type(){
    return true;
}

Status<ssize_t> EventualAllocableDataStorageDriver::eventual_allocate_storage(FileInfo* fi){
    ssize_t bytes_to_alloc = base_storage_driver->sizeof_content(fi);
    if (bytes_to_alloc + placeholder_current_size > max_storage_size)
        return {STORAGE_FULL};

    placeholder_current_size += bytes_to_alloc;
    return {bytes_to_alloc};
}

Status<ssize_t> EventualAllocableDataStorageDriver::eventual_free_storage(FileInfo* fi){
    ssize_t bytes_to_alloc = base_storage_driver->sizeof_content(fi);
    placeholder_current_size -= bytes_to_alloc;
    return {bytes_to_alloc};
}

size_t EventualAllocableDataStorageDriver::placeholder_current_storage_size(){
    return placeholder_current_size;
}

bool EventualAllocableDataStorageDriver::eventual_becomesFull(FileInfo* fi){
    return base_storage_driver->sizeof_content(fi) + placeholder_current_size >= max_storage_size;
}

std::vector<std::string> EventualAllocableDataStorageDriver::configs(){
    auto configs = AllocableDataStorageDriver::configs();
    configs.push_back("Eventual: true\n");
    return configs;
}