//
// Created by dantas on 22/05/21.
//

#include "control_handler.h"
#include "placement_handlers/placement_handler.h"
#include "placement_handlers/ordered_placement/lock_order_placement_handler.h"
#include "placement_handlers/ordered_placement/lock_queue_order_placement_handler.h"
#include "placement_handlers/ordered_placement/single_thread_placement_handler.h"
#include "eviction_handlers/single_thread_eviction_handler.h"
#include "../root/hierarchical_data_plane.h"

ControlHandler::ControlHandler() {
    control_policy = SOLO_PLACEMENT;
    placement_policy = PUSH_DOWN;
    evict_call_type = CLIENT;
    uses_dedicated_thread_pool = true;
    async_placement = true;
}

Status<ssize_t> ControlHandler::place(File *f) {
    if(control_policy == SOLO_PLACEMENT) {
        if(uses_dedicated_thread_pool){
            housekeeper_thread_pool->push([this, f](int id) {
                placement_handler->place(f);
            });
        }
        else{
            placement_handler->place(f);
        }
    }else{
        std::cerr << "Wrong place method called for control_policy" << std::endl;
        exit(1);
    }
    return {SUCCESS};
}

bool ControlHandler::check_placement_validity(){
    return placement_handler->check_placement_validity();
}

bool ControlHandler::check_placement_validity(PlacedFileInfo *pfi) {
    return placement_handler->check_placement_validity(pfi);
}


Status<ssize_t> ControlHandler::place(PrefetchedFile* pf) {
    if(control_policy == SINGLE_THREAD_ORDERED || control_policy == SOLO_PLACEMENT){
        housekeeper_thread_pool->push([this, pf](int id) {
            placement_handler->place(pf);
        });
    }else{
        return placement_handler->place(pf);
    }
    return {SUCCESS};
}

void ControlHandler::base_evict(StrictFileInfo* mfi){
    data_plane_->apply_job_termination();
    //TODO turn the ifs into check_eviction_validity
    if (data_plane_->becomes_full) {
        if(mfi->finish_read() == 0) {
            eviction_handler->evict(mfi);
        }else{
            data_plane_->debug_write("Cannot evict. File will be read again: " + mfi->get_name());
        }
    }
}

void ControlHandler::evict(StrictFileInfo* mfi) {
    if(control_policy == SINGLE_THREAD_ORDERED) {
        housekeeper_thread_pool->push([this, mfi](int id) {
            base_evict(mfi);
        });
    }else if(control_policy == SOLO_PLACEMENT){
        return;
    }else{
        base_evict(mfi);
    }
}

void ControlHandler::prepare_environment() {
    if(uses_dedicated_thread_pool)
        housekeeper_thread_pool = new ctpl::thread_pool(1);

    switch(control_policy){
        case LOCK_ORDERED:
            placement_handler = new LockOrderPlacementHandler(data_plane_, placement_policy, async_placement);
            eviction_handler = new EvictionHandler(data_plane_);
            data_plane_->make_blocking_drivers();
            break;
        case QUEUE_ORDERED:
            placement_handler = new LockQueueOrderPlacementHandler(data_plane_, placement_policy, async_placement);
            eviction_handler = new EvictionHandler(data_plane_);
            data_plane_->make_blocking_drivers();
            break;
        case SINGLE_THREAD_ORDERED: {
            //TODO needs to have thread pool!! (uses_dedicated...)
            auto *stph = new SingleThreadPlacementHandler(data_plane_, placement_policy, async_placement);
            placement_handler = stph;
            eviction_handler = new SingleThreadEvictionHandler(data_plane_, stph, housekeeper_thread_pool);
            data_plane_->make_eventual_drivers();
            break;
        }
        case SOLO_PLACEMENT:
            placement_handler = new PlacementHandler(data_plane_, placement_policy, async_placement);
            eviction_handler = nullptr;
            if(!uses_dedicated_thread_pool){
                data_plane_->make_blocking_drivers();
            }
            break;
    }
}

PlacementPolicy ControlHandler::get_placement_policy(){
    return placement_policy;
}

ControlPolicy ControlHandler::get_control_policy() {
    return control_policy;
}

bool ControlHandler::uses_async_calls() const{
    return uses_dedicated_thread_pool || async_placement;
}

void ControlHandler::change_policies(ControlPolicy cp, PlacementPolicy pp){
    control_policy = cp;
    placement_policy = pp;
}
