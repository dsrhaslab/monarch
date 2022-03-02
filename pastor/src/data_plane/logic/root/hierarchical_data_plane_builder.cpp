//
// Created by dantas on 25/10/20.
//

#include <iostream>
#include <utility>
#include "hierarchical_data_plane_builder.h"


HierarchicalDataPlane* HierarchicalDataPlaneBuilder::build() {
    if (data_plane->total_used_threads == 0){
        data_plane->shared_thread_pool = true;
        data_plane->storage_hierarchy_thread_pools.push_back(new ctpl::thread_pool(1));
        data_plane->total_used_threads = 1;
    }
    return data_plane;
}

HierarchicalDataPlaneBuilder& HierarchicalDataPlaneBuilder::hierarchy(){
    return *this;
}

HierarchicalDataPlaneBuilder& HierarchicalDataPlaneBuilder::configurations(){
    return *this;
}

HierarchicalDataPlaneBuilder& HierarchicalDataPlaneBuilder::policies(){
    return *this;
}

HierarchicalDataPlaneBuilder& HierarchicalDataPlaneBuilder::with_storage_hierarchy(std::vector<std::vector<DataStorageDriver*>> matrix){
    data_plane->storage_hierarchical_matrix = std::move(matrix);
    return *this;
}

HierarchicalDataPlaneBuilder& HierarchicalDataPlaneBuilder::with_storage_hierarchy_pools(std::vector<ctpl::thread_pool*> pools){
    data_plane->storage_hierarchy_thread_pools = std::move(pools);
    return *this;
}

HierarchicalDataPlaneBuilder& HierarchicalDataPlaneBuilder::with_shared_thread_pool(int pool_size){
    data_plane->shared_thread_pool = true;
    data_plane->storage_hierarchy_thread_pools.push_back(new ctpl::thread_pool(pool_size));
    data_plane->total_used_threads += pool_size;
    return *this;
}

/*
HierarchicalDataPlaneBuilder& HierarchicalDataPlaneBuilder::with_housekeeper_thread_pool_size(int size){
    data_plane->housekeeper_thread_pool = new ctpl::thread_pool(size);
    return *this;
}
*/

HierarchicalDataPlaneBuilder& HierarchicalDataPlaneBuilder::with_debug_enabled(const std::string& dir_path, int unique_id){
    data_plane->debug_logger->configure_service(dir_path, unique_id);
    return *this;
}

HierarchicalDataPlaneBuilder& HierarchicalDataPlaneBuilder::with_metadata_container(MetadataContainerService<FileInfo>* mdc){
    data_plane->metadata_container = mdc;
    return *this;
}

HierarchicalDataPlaneBuilder& HierarchicalDataPlaneBuilder::with_client_watch_rate_limit(int size){
    data_plane->rate_limiter = new ClientWatchRateLimiter(size);
    return *this;
}

//TODO fix rate limiters object hierarchy
HierarchicalDataPlaneBuilder& HierarchicalDataPlaneBuilder::with_batch_rate_limit(int size) {
    //data_plane->rate_limiter = new RateLimiter(size);
    return *this;
}

HierarchicalDataPlaneBuilder& HierarchicalDataPlaneBuilder::with_profiling_enabled(ProfilerProxy* pp, ProfilingService* ps){
    data_plane->profiler = pp;
    data_plane->profiling_service = ps;
    return *this;
}

HierarchicalDataPlaneBuilder& HierarchicalDataPlaneBuilder::with_type(const std::string& type){
    data_plane->type = type;
    return *this;
}

HierarchicalDataPlaneBuilder& HierarchicalDataPlaneBuilder::with_storage_synchronization_timeout(int timeout){
    data_plane->storage_sync_timeout = timeout;
    return *this;
}

HierarchicalDataPlaneBuilder& HierarchicalDataPlaneBuilder::with_control_handler(ControlHandlerBuilder* chb){
    chb->with_hierarchical_data_plane(data_plane);
    data_plane->control_handler = chb->build();
    return *this;
}