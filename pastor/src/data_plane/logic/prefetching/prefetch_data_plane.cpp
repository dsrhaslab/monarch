//
// Created by dantas on 26/10/20.
//

#include <thread>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include <cmath>
#include "prefetch_data_plane.h"
#include "prefetch_data_plane_builder.h"
#include "../handlers/control_handler.h"


PrefetchDataPlane::PrefetchDataPlane(HierarchicalDataPlane* rdp) : HierarchicalDataPlane(rdp){
    eviction_percentage = 0.1;
    allocated_samples_index = 0;
    becomes_full = true;
}


//TODO review this
void PrefetchDataPlane::start_prefetching_solo_placement(){
    int iter_size = HierarchicalDataPlane::metadata_container->get_file_count();
    HierarchicalDataPlane::set_total_jobs(iter_size);
    debug_write("Instance with rank "  + std::to_string(rank) + " starting prefetching with iter size: " + std::to_string(iter_size));
    for (int i = 0; i < iter_size; i++){
        int sample_id  = HierarchicalDataPlane::get_list_index(rank, worker_id, i);
        auto* sfi = (StrictFileInfo*)metadata_container->get_metadata(sample_id);
        if(enforce_rate_limit())
            debug_write("Rate limit was enforced");
        HierarchicalDataPlane::apply_job_start();
        debug_write("Prefetching local_index: " + std::to_string(i) + ", sample_id: " + std::to_string(sample_id) + " name: " + sfi->get_name());

        //TODO make reached_stability atomic?
        if(control_handler->check_placement_validity()) {
            prefetch_thread_pool->push([this, sfi, i](int id) {
                if (!sfi->init_prefetch()) {
                    debug_write("File already dealt with  id: " + std::to_string(i) + " name: " + sfi->get_name());
                } else {
                    debug_write("Prefetching id: " + std::to_string(i) + ", name: " + sfi->get_name() + " from level " +
                                std::to_string(sfi->get_storage_level()));
                    auto* f = new PrefetchedFile(sfi, i);
                    HierarchicalDataPlane::read_from_storage(false, f, sfi->get_storage_level());
                    control_handler->place(f);
                }
                HierarchicalDataPlane::apply_job_termination();
            });
        }else{
            HierarchicalDataPlane::apply_job_termination();
        }
    }
    HierarchicalDataPlane::await_termination();
    debug_write("Prefetch ended!");
}

void PrefetchDataPlane::start_prefetching_with_eviction(){
    int iter_size = HierarchicalDataPlane::metadata_container->get_iter_size();
    HierarchicalDataPlane::set_total_jobs(iter_size);
    debug_write("Instance with rank "  + std::to_string(rank) + " starting prefetching with iter size: " + std::to_string(iter_size));
    for (int i = 0; i < iter_size; i++){
        int sample_id  = HierarchicalDataPlane::get_list_index(rank, worker_id, i);
        auto* sfi = (StrictFileInfo*)metadata_container->get_metadata(sample_id);
        if(enforce_rate_limit())
            debug_write("Rate limit was enforced");
        HierarchicalDataPlane::apply_job_start();
        debug_write("Prefetching local_index: " + std::to_string(i) + ", sample_id: " + std::to_string(sample_id) + " name: " + sfi->get_name());

        prefetch_thread_pool->push([this, sfi, i](int id){
            auto* f = new PrefetchedFile(sfi, i);

            if(!sfi->init_prefetch()){
                debug_write("File already dealt with or is in the upper_level id: " + std::to_string(i) + " name: " + sfi->get_name());
                f->set_as_placeholder();
            }else {
                debug_write("Prefetching id: " + std::to_string(i) + ", name: " + sfi->get_name() + " from level " + std::to_string(sfi->get_storage_level()));
                HierarchicalDataPlane::read_from_storage(false, f, sfi->get_storage_level());
            }
            control_handler->place(f);
        });
    }
    HierarchicalDataPlane::await_termination();
    debug_write("Prefetch ended!");
}

void PrefetchDataPlane::start_prefetching() {
    if(control_handler->get_control_policy() == SOLO_PLACEMENT){
        start_prefetching_solo_placement();
    }else{
        start_prefetching_with_eviction();
    }
}

ssize_t PrefetchDataPlane::read(StrictFileInfo* sfi, char* result, uint64_t offset, size_t n){
    if(offset >= sfi->_get_size()) {
        debug_write("Tried to read from offset: " + std::to_string(offset) + " which goes beond file size: " +
                    std::to_string(sfi->_get_size()) + " name: " + sfi->get_name());
        return 0;
    }

    if(read_policy == WAIT_ENFORCED) {
        bool waited = sfi->await_loaded_data(0);
        if (waited)
            debug_write("Client waited for file: " + sfi->get_name());
    }
    debug_write("Read from offset: " + std::to_string(offset) + " n: " + std::to_string(n)
                + " name: " + sfi->get_name() + " from level " + std::to_string(sfi->get_storage_level()));
    File* f = new File(sfi, offset, n);
    Status<ssize_t> s = HierarchicalDataPlane::read_from_storage(true, f, sfi->get_storage_level());
    memcpy(result, f->get_content(), f->get_requested_size());
    delete f;
    if(offset + n >= sfi->_get_size()) {
        control_handler->evict(sfi);
    }
    return s.return_value;
}

ssize_t PrefetchDataPlane::read(const std::string& filename, char* result, uint64_t offset, size_t n){
    auto* mfi = (StrictFileInfo*)(HierarchicalDataPlane::metadata_container->get_metadata(filename));
    return read(mfi, result, offset, n);
}

ssize_t PrefetchDataPlane::read_from_id(int file_id, char* result, uint64_t offset, size_t n){
    auto* mfi = (StrictFileInfo*)(HierarchicalDataPlane::metadata_container->get_metadata(file_id));
    return read(mfi, result, offset, n);
}

ssize_t PrefetchDataPlane::read_from_id(int file_id, char* result){
    auto* mfi = (StrictFileInfo*)(HierarchicalDataPlane::metadata_container->get_metadata(file_id));
    return read(mfi, result, 0, mfi->_get_size());
}

PrefetchDataPlaneBuilder* PrefetchDataPlane::create(HierarchicalDataPlane* rdp) {
    return new PrefetchDataPlaneBuilder{rdp};
}

void PrefetchDataPlane::init(bool transparent_api_){
    HierarchicalDataPlane::init(transparent_api_);
}

void PrefetchDataPlane::start(){
    HierarchicalDataPlane::start();
    std::thread producer_thread(&PrefetchDataPlane::start_prefetching, this);
    producer_thread.detach();
    /* TODO
    synchronization_thread_pool->push([this](int id){
        HierarchicalDataPlane::start_sync_loop(1);
    });
     */
}

std::vector<std::string> PrefetchDataPlane::configs(){
    std::vector<std::string> res;
    res.push_back("- PrefetchModule");
    res.push_back("\tprefetch_thread_pool_size: " + std::to_string(prefetch_thread_pool->size()) );
    res.push_back("\teviction_percentage: " + std::to_string(eviction_percentage) );
    for(const auto& str : HierarchicalDataPlane::configs())
        res.push_back("\t" + str);
    return res;
}

void PrefetchDataPlane::print(){
    if(instance_id == 0)
        for (const auto& str : configs())
            std::cout << str;
}

void PrefetchDataPlane::debug_write(const std::string& msg){
    if(debug_logger->is_activated()) {
        debug_logger->_write("[PrefetchDataPlane] " + msg);
    }
}