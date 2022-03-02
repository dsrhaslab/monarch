//
// Created by dantas on 27/05/21.
//

#include "single_thread_placement_handler.h"
#include "../../../root/hierarchical_data_plane.h"

SingleThreadPlacementHandler::SingleThreadPlacementHandler(HierarchicalDataPlane* data_plane, PlacementPolicy placement_policy, bool async_placement) : OrderedPlacementHandler(data_plane, placement_policy, async_placement){}

Status<ssize_t> SingleThreadPlacementHandler::inorder_place(PrefetchedFile* f){
    if(!f->is_placeholder()) {
        int to_level = 0;
        if(data_plane_->get_alloc_driver(to_level)->becomesFull(f->get_info())) {
            data_plane_->debug_write("Intended driver becomes full with file: " + std::to_string(f->get_request_id())
                                     + " current size: " + std::to_string(data_plane_->get_alloc_driver(to_level)->current_storage_size()));
            return {DELAYED};
        }else {
            data_plane_->debug_write(
                    "Proceeding with storage allocation for file: " + std::to_string(f->get_request_id()));
            OrderedPlacementHandler::place(f);
        }
    }
    allocated_samples_index++;
    return {SUCCESS};
}

void SingleThreadPlacementHandler::flush_postponed(){
    if(!postponed_queue.empty()){
        data_plane_->debug_write("Handling postponed requests");
        long res = postponed_queue.size();
        while(res > 0) {
            auto* f = postponed_queue.front();
            data_plane_->debug_write("On handling postponed requests retrieved request with id: " +std::to_string(f->get_request_id()));
            Status<ssize_t> s = inorder_place(f);
            if(s.state == DELAYED)
                break;
            else{
                res--;
                postponed_queue.pop();
            }
        }
        if(res == 0) {
            data_plane_->debug_write("All postponed requests are treated...resuming delayed requests treatment");
            Status<ssize_t> s = update_queue_and_place();
            if(s.state == SUCCESS) {
                data_plane_->debug_write("After treating postponed requests update queue and place did not return DELAYED. Enforcing rate continuation");
            }
        }
    }
}

Status<ssize_t> SingleThreadPlacementHandler::update_queue_and_place(){
    if(!placement_queue.empty()) {
        auto *f = placement_queue.top();
        if (in_order(f)) {
            data_plane_->debug_write("Placement queue is being updated: found an in order request with id: " + std::to_string(f->get_request_id()));
            Status<ssize_t> s = inorder_place(f);
            placement_queue.pop();
            if(s.state == SUCCESS) {
                data_plane_->debug_write("Delayed element was successfuly added: his timestamp: " + std::to_string(f->get_request_id()));
                return update_queue_and_place();
            }else{
                data_plane_->debug_write("Delayed element will be postponed timestamp: " + std::to_string(f->get_request_id()));
                postponed_queue.push(f);
                return {DELAYED};
            }
        }
    }
    //Success represents an empty queue or no new ordered files
    return {SUCCESS};
}

Status<ssize_t> SingleThreadPlacementHandler::place(PrefetchedFile* f){
    if(in_order(f)){
        data_plane_->debug_write("Found an in order file with timestamp: " + std::to_string(f->get_request_id()));
        Status<ssize_t> s = inorder_place(f);
        if(s.state == SUCCESS) {
            s = update_queue_and_place();
        }else{
            data_plane_->debug_write("Postponing placement request for file with timestamp: " + std::to_string(f->get_request_id()));
            postponed_queue.push(f);
        }
        return s;
    }
    else {
        data_plane_->debug_write("Found an out of order placement request, timestamp: "
                                 + std::to_string(f->get_request_id()) + " current index is: " + std::to_string(allocated_samples_index) );
        placement_queue.push(f);
        return {DELAYED};
    }
}