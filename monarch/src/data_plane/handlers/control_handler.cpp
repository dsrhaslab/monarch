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

ControlHandler::ControlHandler(HierarchicalDataPlane* data_plane, ControlPolicy cp, PlacementPolicy pp) : data_plane_(data_plane),
control_policy(cp), placement_policy(pp) {}

Status ControlHandler::place(File *f) {
    if(control_policy == SOLO_PLACEMENT) {
        data_plane_->housekeeper_thread_pool->push([this, f](int id) {
            placement_handler->place(f);
        });
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


Status ControlHandler::place(PrefetchedFile* pf) {
    if(control_policy == SINGLE_THREAD_ORDERED || control_policy == SOLO_PLACEMENT){
        data_plane_->housekeeper_thread_pool->push([this, pf](int id) {
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
        data_plane_->housekeeper_thread_pool->push([this, mfi](int id) {
            base_evict(mfi);
        });
    }else if(control_policy == SOLO_PLACEMENT){
        return;
    }else{
        base_evict(mfi);
    }
}

void ControlHandler::prepare_environment() {
    switch(control_policy){
        case LOCK_ORDERED:
            placement_handler = new LockOrderPlacementHandler(data_plane_, placement_policy);
            eviction_handler = new EvictionHandler(data_plane_);
            data_plane_->make_blocking_drivers();
            break;
        case QUEUE_ORDERED:
            placement_handler = new LockQueueOrderPlacementHandler(data_plane_, placement_policy);
            eviction_handler = new EvictionHandler(data_plane_);
            data_plane_->make_blocking_drivers();
            break;
        case SINGLE_THREAD_ORDERED: {
            auto *stph = new SingleThreadPlacementHandler(data_plane_, placement_policy);
            placement_handler = stph;
            eviction_handler = new SingleThreadEvictionHandler(data_plane_, stph);
            data_plane_->make_eventual_drivers();
            break;
        }
        case SOLO_PLACEMENT:
            placement_handler = new PlacementHandler(data_plane_, placement_policy);
            eviction_handler = nullptr;
            break;
    }
}

void ControlHandler::change_policies(ControlPolicy cp, PlacementPolicy pp){
    control_policy = cp;
    placement_policy = pp;
}
