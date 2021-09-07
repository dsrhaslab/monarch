//
// Created by dantas on 19/10/20.
//

#include <iostream>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <chrono>

#include "hierarchical_data_plane.h"
#include "hierarchical_data_plane_builder.h"
#include "../handlers/control_handler.h"

HierarchicalDataPlaneBuilder* HierarchicalDataPlane::create(int instance_id_, int world_size_, int number_of_workers, int hierarchy_size){
    return new HierarchicalDataPlaneBuilder{instance_id_, world_size_, number_of_workers, hierarchy_size};
};

HierarchicalDataPlane::HierarchicalDataPlane(int id, int ws, int nw, int hierarchy_size) {
    neighbours_index = 0;
    placed_samples = 0;
    instance_id = id;
    rank = -1;
    world_size = ws;
    worker_id = -1;
    num_workers = nw;
    storage_sync_timeout = 15;
    reached_stability = false;
    housekeeper_thread_pool = new ctpl::thread_pool(1);
    synchronization_thread_pool = new ctpl::thread_pool(1);
    storage_hierarchy_size = hierarchy_size;
    total_used_threads = 0;
    shared_thread_pool = false;
    debug_logger = new Logger();
    rate_limiter = new ClientWatchRateLimiter(-1);
    control_policy = SOLO_PLACEMENT;
    placement_policy = PUSH_DOWN;
    control_handler = new ControlHandler(this, control_policy, placement_policy);
    profiler = nullptr;
}

HierarchicalDataPlane::HierarchicalDataPlane(HierarchicalDataPlane* hdp){
    neighbours_index = 0;
    placed_samples = 0;
    storage_sync_timeout = hdp->storage_sync_timeout;
    instance_id = hdp->instance_id;
    matrix_index = hdp->matrix_index;
    rank = hdp->rank;
    world_size = hdp->world_size;
    worker_id = hdp->worker_id;
    num_workers = hdp->num_workers;
    reached_stability = false;
    housekeeper_thread_pool = hdp->housekeeper_thread_pool;
    synchronization_thread_pool = hdp->synchronization_thread_pool;
    storage_hierarchical_matrix = hdp->storage_hierarchical_matrix;
    storage_hierarchy_size = hdp->storage_hierarchy_size;
    metadata_container = hdp->metadata_container;
    storage_hierarchy_thread_pools = hdp->storage_hierarchy_thread_pools;
    total_used_threads = hdp->total_used_threads;
    shared_thread_pool = hdp->shared_thread_pool;
    debug_logger = hdp->debug_logger;
    rate_limiter = hdp->rate_limiter;
    profiler = hdp->profiler;
    control_handler = hdp->control_handler;
    type = hdp->type;
}

void HierarchicalDataPlane::start_sync_loop(int offset){
    if (world_size == 1 && num_workers == 1)
        return;
    while(placed_samples != metadata_container->get_iter_size()) {
        sleep(storage_sync_timeout);
        synchronize_storages(offset);
    }
}

ctpl::thread_pool* HierarchicalDataPlane::t_pool(int storage_index){
    if (!shared_thread_pool)
        return storage_hierarchy_thread_pools[storage_index];
    return storage_hierarchy_thread_pools[0];
}

int HierarchicalDataPlane::get_storage_hierarchical_matrix_index(int rank_, int worker_id_){
    return worker_id_ + rank_ * num_workers;
}

//ignores worker_id for now
//TODO stability should be reached when all storage level are 99.99% full and not related to index.
//The if is kind of strange, but for now does no harm
int HierarchicalDataPlane::get_list_index(int rank_, int worker_id_, int local_index){
    if(local_index == metadata_container->get_file_count() - 1)
        reached_stability = true;
    return metadata_container->get_id(rank_, local_index);
}

bool HierarchicalDataPlane::is_file_system(int level){
    return storage_hierarchical_matrix[matrix_index][level]->file_system_type();
}

FileSystemDriver* HierarchicalDataPlane::get_fs_driver(int level){
    auto* driver = storage_hierarchical_matrix[matrix_index][level];
    if(driver->alloc_type()){
        return (FileSystemDriver*)(((AllocableDataStorageDriver*)driver)->get_base_storage_driver());
    }
    return (FileSystemDriver*)driver;
}

bool HierarchicalDataPlane::is_allocable(int rank_, int worker_id_, int level){
    int line = get_storage_hierarchical_matrix_index(rank_, worker_id_);
    return storage_hierarchical_matrix[line][level]->alloc_type();
}

bool HierarchicalDataPlane::is_allocable(int level){
    return storage_hierarchical_matrix[matrix_index][level]->alloc_type();
}

bool HierarchicalDataPlane::is_blocking(int level){
    auto* driver = storage_hierarchical_matrix[matrix_index][level];
    if(driver->alloc_type()){
        return ((AllocableDataStorageDriver*)driver)->blocking_type();
    }
    return false;
}

bool HierarchicalDataPlane::is_eventual(int level){
    auto* driver = storage_hierarchical_matrix[matrix_index][level];
    if(driver->alloc_type()){
        return ((AllocableDataStorageDriver*)driver)->eventual_type();
    }
    return false;
}

AllocableDataStorageDriver* HierarchicalDataPlane::get_alloc_driver(int rank_, int worker_id_, int level){
    int line = get_storage_hierarchical_matrix_index(rank_, worker_id_);
    return (AllocableDataStorageDriver*)storage_hierarchical_matrix[line][level];
}

AllocableDataStorageDriver* HierarchicalDataPlane::get_alloc_driver(int level){
    return (AllocableDataStorageDriver*)storage_hierarchical_matrix[matrix_index][level];
}

BlockingAllocableDataStorageDriver* HierarchicalDataPlane::get_blocking_alloc_driver(int level){
    return (BlockingAllocableDataStorageDriver*)storage_hierarchical_matrix[matrix_index][level];
}

DataStorageDriver* HierarchicalDataPlane::get_driver(int rank_, int worker_id_, int level){
    int line = get_storage_hierarchical_matrix_index(rank_, worker_id_);
    return storage_hierarchical_matrix[line][level];
}

DataStorageDriver* HierarchicalDataPlane::get_driver(int level){
    return storage_hierarchical_matrix[matrix_index][level];
}

//TODO does not count level 0 if it's in-memory type with prefetch
int HierarchicalDataPlane::free_level(int rank_, int worker_id_, FileInfo* fi, int offset){
    for(int i = offset; i < storage_hierarchy_size - 1; i++)
        if(is_allocable(rank_, worker_id_, i)){
            auto* driver = get_alloc_driver(rank_, worker_id_, i);
            if((type != "prefetch_enabled" || !driver->in_memory_type()) && !driver->becomesFull(fi))
                return i;
        }
    return -1;
}

//TODO more generic
int HierarchicalDataPlane::alloc_free_level(FileInfo* fi, int offset){
    for(int i = offset; i < storage_hierarchy_size - 1; i++) {
        if (get_alloc_driver(i)->conditional_allocate_storage(fi).state != STORAGE_FULL) {
            return i;
        }
    }
    return -1;
}

int HierarchicalDataPlane::free_level(FileInfo* fi, int offset){
    for(int i = offset; i < storage_hierarchy_size - 1; i++) {
        if (!get_alloc_driver(i)->becomesFull(fi)) {
            return i;
        }
    }
    return -1;
}

EventualAllocableDataStorageDriver* HierarchicalDataPlane::get_eventual_alloc_driver(int level){
    return (EventualAllocableDataStorageDriver*)storage_hierarchical_matrix[matrix_index][level];
}

int HierarchicalDataPlane::eventual_free_level(FileInfo* fi, int offset){
    for(int i = offset; i < storage_hierarchy_size - 1; i++) {
        if (!get_eventual_alloc_driver(i)->eventual_becomesFull(fi)) {
            return i;
        }
    }
    return -1;
}

//TODO doesn't support in-memory type due to eviction policy
//Its thread since the housekeeper does not interact with the neighbours storages
void HierarchicalDataPlane::synchronize_storages(int offset){
    if(!reached_stability){
        for(int i = 0; i < placed_samples; i++){
            for(int r = 0; r < world_size; r++){
                for(int w = 0; w < num_workers; w++){
                    //dont update your own storage
                    if(get_storage_hierarchical_matrix_index(r, w) != matrix_index) {
                        int index = get_list_index(r, w, neighbours_index);
                        auto *fi = metadata_container->get_metadata_from_ordered_id(r, index);
                        if (fi->get_storage_level() == storage_hierarchy_size - 1) {
                            int level = free_level(r, w, fi, offset);
                            if (level > 0) {
                                get_alloc_driver(r, w, level)->allocate_storage(fi);
                                fi->loaded_to(level);
                            }
                        }
                    }
                }
            }
            neighbours_index++;
        }
        debug_write("During synchronization neighbours_index was updated to:" + std::to_string(neighbours_index));
    }
}

std::string HierarchicalDataPlane::decode_filename(std::string full_path){
    return full_path.erase(0, get_driver(storage_hierarchy_size -1)->prefix().length() + 1);
}

ssize_t HierarchicalDataPlane::base_read(FileInfo* fi, char* result, uint64_t offset, size_t n){
    if(offset >= fi->_get_size()) {
        if(debug_logger->is_activated())
            debug_write("Tried to read from offset: " + std::to_string(offset) + " which goes beond file size: " +
                    std::to_string(fi->_get_size()) + " name: " + fi->get_name());
        return 0;
    }

    int storage_level = fi->get_storage_level();
    size_t requested_size = n + offset > fi->_get_size() ? fi->_get_size() - offset : n;

    if(debug_logger->is_activated())
        debug_write("client reading from level " +
                    std::to_string(storage_level) + 
                    " file " + fi->get_name() + 
                    " with offset: " +
                    std::to_string(offset) + " and size: " + std::to_string(requested_size));

    if(fi->has_shareable_file_descriptors() && is_file_system(storage_level)){
        get_fs_driver(storage_level)->open_descriptor(fi, true);
    }
    Status read_status = read_from_storage(fi, result, offset, requested_size, storage_level);
    //Started reading must be called after read so that the first read opens de file if fs driver.
    if(read_status.state == SUCCESS) {
        if(control_handler->check_placement_validity(dynamic_cast<PlacedFileInfo*>(fi))){
            auto* f = new File(fi, offset, requested_size);
            if(f->is_full_read()){
                memcpy(f->get_content(), result, requested_size);
            }
            control_handler->place(f);
        }
    }
    //TODO this mechanism needs to be added with prefetch + relaxed read
    if(fi->has_shareable_file_descriptors()){
        if(offset + requested_size >= fi->_get_size() && is_file_system(storage_level)){
            fi->client_ended_read(storage_level);
            get_fs_driver(storage_level)->close_descriptor(fi);
        }
        if(fi->storage_changed()){
            //To prevent close without open
            if(fi->get_file_descriptor(fi->get_last_storage_read()) != -1) {
                fi->client_ended_read(fi->get_last_storage_read());
                get_fs_driver(fi->get_last_storage_read())->close_descriptor(fi);
            }
            fi->update_last_storage_read();
        }
    }
    return read_status.bytes_size;
}

ssize_t HierarchicalDataPlane::read(FileInfo* fi, char* result, uint64_t offset, size_t n){
    ssize_t bytes_size = 0;
    if(profiler->is_activated()) {
        auto *read_submission = new ReadSubmission();
        read_submission->r_start = std::chrono::high_resolution_clock::now();

        bytes_size = base_read(fi, result, offset, n);

        read_submission->r_end = std::chrono::high_resolution_clock::now();
        read_submission->n = n;
        profiler->submit_client_read(read_submission);
    }else{
        bytes_size = base_read(fi, result, offset, n);
    }
    return bytes_size;
}

ssize_t HierarchicalDataPlane::read(const std::string& filename, char* result, uint64_t offset, size_t n) {
    auto* fi = metadata_container->get_metadata(filename);
    return read(fi, result, offset, n);
}

ssize_t HierarchicalDataPlane::read_from_id(int file_id, char* result, uint64_t offset, size_t n) {
    auto* fi = metadata_container->get_metadata(file_id);
    return read(fi, result, offset, n);
}

ssize_t HierarchicalDataPlane::read_from_id(int file_id, char* result){
    auto* fi = metadata_container->get_metadata(file_id);
    return read(fi, result, 0, fi->_get_size());
}

int HierarchicalDataPlane::get_target_class_from_id(int id) {
    return metadata_container->get_metadata(id)->get_target();
}

size_t HierarchicalDataPlane::get_file_size(const std::string &filename){
    return metadata_container->get_metadata(filename)->_get_size();
}

size_t HierarchicalDataPlane::get_file_size_from_id(int id){
    return metadata_container->get_metadata(id)->_get_size();
}

int HierarchicalDataPlane::get_target_class(const std::string &filename){
    return metadata_container->get_metadata(filename)->get_target();
}

//TODO encapsulate inside other member function to enable frequency

Status HierarchicalDataPlane::write(File* f, int level){
    Status s = SUCCESS;
    if(profiler->is_activated()){
        auto st = std::chrono::high_resolution_clock::now();
        s = storage_hierarchical_matrix[matrix_index][level]->write(f);
        auto et = std::chrono::high_resolution_clock::now();
        profiler->submit_write_on_storage(st, et, level, f->get_requested_size());
    }else{
        s = storage_hierarchical_matrix[matrix_index][level]->write(f);
    }
    return s;
}

Status HierarchicalDataPlane::base_read_from_storage(FileInfo* fi, char* result, uint64_t offset, size_t n, int level){
    Status s = NOT_FOUND;
    int found_level = level;
    while(s.state == NOT_FOUND && found_level < storage_hierarchy_size){
        s = storage_hierarchical_matrix[matrix_index][found_level]->read(fi, result, offset, n);
        if(s.state == NOT_FOUND)
            found_level += 1;
    }
    if(s.state == NOT_FOUND){
        std::cerr << "File : " << fi->get_name() << "could not be found anywhere. Request level: " + std::to_string(level) + " max level permitted: " +
        std::to_string(found_level) + "exiting...\n";
        exit(1);
    }

    if (found_level != level) {
        s = MISS;
        //f->get_info()->set_storage_level(found_level);
        if(debug_logger->is_activated())
            debug_write("Cache miss. Target level: " + std::to_string(level) + " Actual level: " + std::to_string(found_level));
    }
    return s;
}

Status HierarchicalDataPlane::read_from_storage(FileInfo* fi, char* result, uint64_t offset, size_t n, int level){
    Status s = NOT_FOUND;
    if(profiler->is_activated()){
        auto st = std::chrono::high_resolution_clock::now();
        s = base_read_from_storage(fi, result, offset, n, level);
        auto et = std::chrono::high_resolution_clock::now();
        profiler->submit_read_on_storage(st, et, level, n);
    }else{
        s = base_read_from_storage(fi, result, offset, n, level);
    }
    return s;
}

Status HierarchicalDataPlane::read_from_storage(File* f, int level){
    return read_from_storage(f->get_info(), f->get_content(), f->get_offset(), f->get_requested_size(), level);
}

Status HierarchicalDataPlane::remove(FileInfo* fi, int level){
    return storage_hierarchical_matrix[matrix_index][level]->remove(fi);
}

File* HierarchicalDataPlane::remove_for_copy(FileInfo* fi, int level){
    return storage_hierarchical_matrix[matrix_index][level]->remove_for_copy(fi);
}

bool HierarchicalDataPlane::enforce_rate_limit(){
    if(rate_limiter != nullptr)
        return rate_limiter->rate_limit();
    return false;
}

void HierarchicalDataPlane::enforce_rate_brake(int new_brake_id){
    std::cerr << "Method not available" << std::endl;
    exit(1);
    //if(rate_limiter != nullptr) {
     //   rate_limiter->pull_brake(new_brake_id);
    ///}
}

void HierarchicalDataPlane::enforce_rate_continuation(int new_brake_release_id){
    std::cerr << "Method not available" << std::endl;
    exit(1);
    //if(rate_limiter != nullptr)
      //  rate_limiter->release_brake(new_brake_release_id);
}

void HierarchicalDataPlane::apply_job_termination() {
    if(rate_limiter != nullptr)
        rate_limiter->apply_job_termination();
}

void HierarchicalDataPlane::apply_job_start(){
    if(rate_limiter != nullptr)
        rate_limiter->apply_job_start();
}

void HierarchicalDataPlane::await_termination(){
    if(rate_limiter != nullptr)
        rate_limiter->await_termination();
}

void HierarchicalDataPlane::set_total_jobs(int iter_size){
    if(rate_limiter != nullptr)
        rate_limiter->set_total_jobs(iter_size);
}

void HierarchicalDataPlane::init(){
    if(debug_logger->is_activated())
        debug_write("Setting metadata container instance id...");
    if(world_size > 1 || num_workers > 1) {
        metadata_container->make_partition(rank, world_size, worker_id, num_workers);
        if(debug_logger->is_activated())
            debug_write("Metadatada container has partitioned file count set to: " + std::to_string(metadata_container->get_file_count()));

        for(int w = 0; w < num_workers; w++){
            for(int r = 0; r < world_size; r++){
                for(int i = 0; i < storage_hierarchy_size - 1; i++){
                    if (is_allocable(r, w, i)) {
                        auto *driver = get_alloc_driver(r, w, i);
                        size_t total_size = driver->get_max_storage_size();
                        size_t new_size = std::floor(total_size / (num_workers * world_size));
                        driver->resize(new_size);
                    }
                }
            }
        }
        if(debug_logger->is_activated())
            debug_write("Allocable drivers max_storage_size have been resized according to the given partition");
    }
    if(instance_id == 0){
        //ignore source level
        auto dirs = metadata_container->get_dirs_for_environment();
        for(int i = 0; i < storage_hierarchy_size - 1; i++){
            (storage_hierarchical_matrix[matrix_index][i])->create_environment(dirs);
        }
        if(debug_logger->is_activated())
            debug_write("Instance 0 created storage environment");
    }

    //TODO take this out of here. logic in the wrong place
    if(storage_hierarchy_size > 1) {
        if (placement_policy == PUSH_DOWN) {
            size_t full_capacity = 0;
            for (int i = 0; i < storage_hierarchy_size - 1; i++) {
                if (is_allocable(i)) {
                    full_capacity += get_alloc_driver(i)->get_max_storage_size();
                }
            }
            becomes_full = metadata_container->get_full_size() > full_capacity;
        } else {
            becomes_full = metadata_container->get_full_size() > get_alloc_driver(0)->get_max_storage_size();
        }
        std::string b_f = becomes_full ? "true" : "false";
        if(debug_logger->is_activated())
            debug_write("First storage level becomes full: " + b_f );
    }
}

void HierarchicalDataPlane::start(){
    if(rank == -1) {
        debug_write("Adjusting rank...");
        rank = 0;
    }
    if(worker_id == -1){
        debug_write("Adjusting worker_id");
        worker_id = 0;
    }

    //TODO this semantic is weird. Make it better!
    control_handler->prepare_environment();
    /* TODO
    synchronization_thread_pool->push([this](int id){start_sync_loop(0);});
     */
}

//TODO use decent method -> stats.h
std::vector<std::string> HierarchicalDataPlane::configs() {
    std::vector<std::string> res;
    res.push_back("- HierarchicalModule\n");
    res.push_back("\t- hierarchy " + std::to_string(storage_hierarchy_size) + "\n");
    res.push_back("\t\thierarchy_size: " + std::to_string(storage_hierarchy_size) + "\n");
    std::string b = shared_thread_pool ? "true" : "false";
    res.push_back("\t\tshared_pool: " + b + "\n");
    res.push_back("\t\ttotal_used_threads: " + std::to_string(total_used_threads) + "\n");

    for(int i = 0; i < storage_hierarchy_size; i++){
        res.push_back("\t\t- storage_driver_" + std::to_string(i) + ":\n");
        if(!shared_thread_pool){
            res.push_back("\t\t\tthread_pool_size: " + std::to_string(storage_hierarchy_thread_pools[i]->size()) + "\n");
        }

        auto* driver = storage_hierarchical_matrix[matrix_index][i];
        for(const auto& str : driver->configs())
            res.push_back("\t\t\t" + str);
    }

    //for(const auto& str : metadata_container->configs())
      //  res.push_back("\t" + str);

    return res;
}

void HierarchicalDataPlane::print(){
    if(instance_id == 0)
        for (const auto &str : configs())
            std::cout << str;
}

int HierarchicalDataPlane::get_file_count(){
    return metadata_container->get_file_count();
}

void HierarchicalDataPlane::make_blocking_drivers(){
    for(int i = 0; i < storage_hierarchy_size - 1; i++){
        if(is_allocable(i)){
            storage_hierarchical_matrix[matrix_index][i] = new BlockingAllocableDataStorageDriver(get_alloc_driver(i));
        }
    }
}

void HierarchicalDataPlane::make_eventual_drivers(){
    for(int i = 0; i < storage_hierarchy_size - 1; i++){
        if(is_allocable(i)){
            storage_hierarchical_matrix[matrix_index][i] = new EventualAllocableDataStorageDriver(get_alloc_driver(i));
        }
    }
}

CollectedStats* HierarchicalDataPlane::collect_statistics(){
    return profiler->collect();
}

void HierarchicalDataPlane::debug_write(const std::string& msg){
    debug_logger->_write("[HierarchicalDataPlane] " + msg);
}

void HierarchicalDataPlane::set_distributed_params(int rank_, int worker_id_){
    HierarchicalDataPlane::rank = rank_;
    HierarchicalDataPlane::worker_id = worker_id_;
    HierarchicalDataPlane::matrix_index = get_storage_hierarchical_matrix_index(rank, worker_id);
}

/*//TODO only possible if char** is passed as argument
 * if(read_status.state == SUCCESS) {
        int level = free_level(f->get_info());
        if (level >= 0){
            memcpy(result, f->get_content(), f->get_requested_size());
            //result is in buffer, writter thread will manage the created file deletion
            housekeeper_thread_pool->push([this, f](int id) { handle_placement(f); });
        }else
            result = f->get_content();
    }
 */