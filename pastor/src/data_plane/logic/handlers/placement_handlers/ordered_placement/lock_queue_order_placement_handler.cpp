//
// Created by dantas on 19/05/21.
//

#include "lock_queue_order_placement_handler.h"

#include "../../../root/hierarchical_data_plane.h"

LockQueueOrderPlacementHandler::LockQueueOrderPlacementHandler(HierarchicalDataPlane* data_plane, PlacementPolicy placement_policy, bool async_placement) : OrderedPlacementHandler(data_plane, placement_policy, async_placement) {}

Status<ssize_t> LockQueueOrderPlacementHandler::inorder_place(PrefetchedFile* f){
    if(!f->is_placeholder()) {
        OrderedPlacementHandler::place(f);
    }
    allocated_samples_index++;
    return {SUCCESS};
}

Status<ssize_t> LockQueueOrderPlacementHandler::update_queue_and_place(std::unique_lock<std::mutex> ul){
    if(!placement_queue.empty()) {
        auto *f = placement_queue.top();
        if (in_order(f)) {
            data_plane_->debug_write("Placement queue is being updated: found an in order request with id: " +
                                     std::to_string(f->get_request_id()));
            inorder_place(f);
            placement_queue.pop();
            data_plane_->debug_write(
                    "Delayed element was successfuly added: his timestamp: " + std::to_string(f->get_request_id()));
            return update_queue_and_place(std::move(ul));
        }
    }
    ul.unlock();
    //Success represents an empty queue or no new ordered files
    return {SUCCESS};
}

Status<ssize_t> LockQueueOrderPlacementHandler::place(PrefetchedFile* f){
    std::unique_lock<std::mutex> ul(mutex);
    if(in_order(f)){
        data_plane_->debug_write("Found an in order file with timestamp: " + std::to_string(f->get_request_id()));
        Status<ssize_t> s = inorder_place(f);
        s = update_queue_and_place(std::move(ul));
        return s;
    }
    else {
        data_plane_->debug_write("Found an out of order placement request, timestamp: "
            + std::to_string(f->get_request_id()) + " current index is: " + std::to_string(allocated_samples_index) );
        placement_queue.push(f);
        return {DELAYED};
    }
}