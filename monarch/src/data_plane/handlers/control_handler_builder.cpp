//
// Created by dantas on 16/08/21.
//

#include "control_handler_builder.h"

ControlHandlerBuilder::ControlHandlerBuilder(){
    control_handler = new ControlHandler();
}

ControlHandler* ControlHandlerBuilder::build(){
    return control_handler;
}

ControlHandlerBuilder& ControlHandlerBuilder::with_control_policy(ControlPolicy cp){
    control_handler->control_policy = cp;
    return *this;
}

ControlHandlerBuilder& ControlHandlerBuilder::with_placement_policy(PlacementPolicy pp){
    control_handler->placement_policy = pp;
    return *this;
}

/*
ControlHandlerBuilder& ControlHandlerBuilder::with_evict_call_type(EvictCallType ect){
    control_handler->evict_call_type = ect;
    return *this;
}
*/

ControlHandlerBuilder& ControlHandlerBuilder::with_dedicated_thread_pool(bool value){
    control_handler->uses_dedicated_thread_pool = value;
    return *this;
}

ControlHandlerBuilder& ControlHandlerBuilder::with_async_placement(bool value){
    control_handler->async_placement = value;
    return *this;
}