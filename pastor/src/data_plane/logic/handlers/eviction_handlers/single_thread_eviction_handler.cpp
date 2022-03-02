//
// Created by dantas on 20/05/21.
//

#include "single_thread_eviction_handler.h"
#include "../placement_handlers/ordered_placement/single_thread_placement_handler.h"
#include "../../root/hierarchical_data_plane.h"
#include "../../metadata/prefetched_file.h"

SingleThreadEvictionHandler::SingleThreadEvictionHandler(HierarchicalDataPlane* data_plane, SingleThreadPlacementHandler* placement_handler, ctpl::thread_pool* housekeeper_thread_pool)
    : EvictionHandler(data_plane), placement_handler_(placement_handler){
    housekeeper_thread_pool_ = housekeeper_thread_pool;
}

//TODO free needs to consider temporary value
//TODO on the other endpoint (placement) writing considers the true value
//TODO put request on a queue and relay the queue index to the work executor
//TODO executor marks that entry with DONE!
//TODO LATER ON in a batch like manner this thread will apply changes
//TODO FORCE EVICT might be added and triggered by placement and by sharing a dispatcher (batch enforcer) with this thread
//TODO for placement -> Stick with same logic but using a dispatcher would be nice (enables reordering(less out of order) and better flow)
//TODO for placement one can just allocate first and then read + write without locking for the latter operations

void SingleThreadEvictionHandler::persist_changes(StrictFileInfo* sfi, int from, int to){
    housekeeper_thread_pool_->push([this, sfi, from, to](int id) {
        data_plane_->get_alloc_driver(from)->free_storage(sfi);
        if(to > -1 && to != sfi->get_staging_level()){
            data_plane_->get_alloc_driver(to)->allocate_storage(sfi);
        }
        placement_handler_->flush_postponed();
    });
}

void SingleThreadEvictionHandler::evict(StrictFileInfo* sfi){
    int fl = data_plane_->eventual_free_level(sfi, 1);
    int current_level = sfi->get_storage_level();
    if(fl > 0){
        data_plane_->get_eventual_alloc_driver(fl)->eventual_allocate_storage(sfi);
        data_plane_->get_eventual_alloc_driver(current_level)->eventual_free_storage(sfi);
        data_plane_->t_pool(current_level)->push([this, sfi, fl](int id){
            EvictionHandler::eviction(sfi, fl);
        });
    }else{
        data_plane_->get_eventual_alloc_driver(current_level)->eventual_free_storage(sfi);
        data_plane_->t_pool(current_level)->push([this, sfi](int id){
            EvictionHandler::eviction(sfi);
        });
    }
}
