//
// Created by dantas on 19/05/21.
//

#include "lock_order_placement_handler.h"

#include "../../../root/hierarchical_data_plane.h"

LockOrderPlacementHandler::LockOrderPlacementHandler(HierarchicalDataPlane* data_plane, PlacementPolicy placement_policy, bool async_placement) : OrderedPlacementHandler(data_plane, placement_policy, async_placement) {}

Status<ssize_t> LockOrderPlacementHandler::place(PrefetchedFile* f){
    std::unique_lock<std::mutex> ul(mutex);
    in_order_condition.wait(ul, [this, f]() { return in_order(f); });
    allocated_samples_index++;
    if(!f->is_placeholder()) {
        data_plane_->debug_write("In order request updating allocated index to: " + std::to_string(allocated_samples_index));
        data_plane_->debug_write("Proceeding with storage allocation for file: " + std::to_string(f->get_request_id()));
        OrderedPlacementHandler::place(f);
        //targeted_placement aquires a different lock so ul is not released if storage is full which is kinda good for us, since it stops the prefetching.
        //write is permanent and changes will be seen by other threads. Can release lock and notify. Write is still async
        ul.unlock();
        in_order_condition.notify_all();
    }else{
        data_plane_->debug_write("Placeholder request updating allocated index to: " + std::to_string(allocated_samples_index));
        ul.unlock();
        in_order_condition.notify_all();
    }
    return {SUCCESS};
}