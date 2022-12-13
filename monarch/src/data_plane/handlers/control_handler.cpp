//
// Created by dantas on 22/05/21.
//

#include <iostream>

#include "../stages/monarch.h"
#include "control_handler.h"
#include "control_handler_builder.h"
#include "placement_handlers/placement_handler.h"

ControlHandlerBuilder ControlHandler::create(){
    return ControlHandlerBuilder{};
}

ControlHandler::ControlHandler() {
    control_policy = ControlPolicy::SOLO_PLACEMENT;
    placement_policy = PlacementPolicy::PUSH_DOWN;
    //evict_call_policy = EvictCallPolicy::CLIENT;
    uses_dedicated_thread_pool = true;
    async_placement = true;
}

Status<ssize_t> ControlHandler::place(File *f) {
    if(control_policy == ControlPolicy::SOLO_PLACEMENT) {
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

bool ControlHandler::check_placement_validity(Info* i) {
    return placement_handler->check_placement_validity(i);
}

StorageDriverStateType ControlHandler::prepare_environment(Monarch* monarch) {
    //Set Monarch
    monarch_ = monarch;
    if(uses_dedicated_thread_pool) {
        housekeeper_thread_pool = new ctpl::thread_pool(1);
    }

    switch(control_policy){
        /*
        case ControlPolicy::LOCK_ORDERED: {
            placement_handler = new LockOrderPlacementHandler(monarch_, placement_policy, async_placement);
            eviction_handler = new EvictionHandler(monarch_);
            return StorageDriverWrapperType::BLOCKING;
        }
        case ControlPolicy::QUEUE_ORDERED: {
            placement_handler = new LockQueueOrderPlacementHandler(monarch_, placement_policy, async_placement);
            eviction_handler = new EvictionHandler(monarch_);
            return StorageDriverWrapperType::BLOCKING;
        }
        case ControlPolicy::SINGLE_THREAD_ORDERED: {
            //TODO needs to have thread pool!! (uses_dedicated...)
            auto *stph = new SingleThreadPlacementHandler(monarch_, placement_policy, async_placement);
            placement_handler = stph;
            eviction_handler = new SingleThreadEvictionHandler(monarch_, stph, housekeeper_thread_pool);
            return StorageDriverWrapperType::EVENTUAL;
        }
        */
        case ControlPolicy::SOLO_PLACEMENT: {
            placement_handler = new PlacementHandler(monarch_, placement_policy, async_placement);
            if(!uses_dedicated_thread_pool){
                return StorageDriverStateType::BLOCKING;
            }
            return StorageDriverStateType::ALLOCABLE;
        }
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

void ControlHandler::set_monarch(Monarch* monarch){
    monarch_ = monarch;
}
