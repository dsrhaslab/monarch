//
// Created by dantas on 16/08/21.
//

#ifndef THESIS_CONTROL_HANDLER_BUILDER_H
#define THESIS_CONTROL_HANDLER_BUILDER_H

#include "control_handler.h"

class HierarchicalDataPlane;

class ControlHandlerBuilder {
private:
    ControlHandler* control_handler;

public:
    ControlHandlerBuilder(){
        control_handler = new ControlHandler();
    }

    ControlHandler* build(){
        return control_handler;
    }

    ControlHandlerBuilder& with_control_policy(ControlPolicy cp){
        control_handler->control_policy = cp;
        return *this;
    }

    ControlHandlerBuilder& with_placement_policy(PlacementPolicy pp){
        control_handler->placement_policy = pp;
        return *this;
    }

    ControlHandlerBuilder& with_evict_call_type(EvictCallType ect){
        control_handler->evict_call_type = ect;
        return *this;
    }

    ControlHandlerBuilder& with_dedicated_thread_pool(bool value){
        control_handler->uses_dedicated_thread_pool = value;
        return *this;
    }

    ControlHandlerBuilder& with_async_placement(bool value){
        control_handler->async_placement = value;
        return *this;
    }

    ControlHandlerBuilder& with_hierarchical_data_plane(HierarchicalDataPlane* hdp){
        control_handler->data_plane_ = hdp;
        return *this;
    }
};

#endif //THESIS_CONTROL_HANDLER_BUILDER_H
