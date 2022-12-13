//
// Created by dantas on 16/08/21.
//

#ifndef MONARCH_CONTROL_HANDLER_BUILDER_H
#define MONARCH_CONTROL_HANDLER_BUILDER_H

#include "control_handler.h"

class ControlHandlerBuilder {
private:
    ControlHandler* control_handler;

public:
    ControlHandlerBuilder();

    ControlHandler* build();

    ControlHandlerBuilder& with_control_policy(ControlPolicy cp);

    ControlHandlerBuilder& with_placement_policy(PlacementPolicy pp);

    //ControlHandlerBuilder& with_evict_call_type(EvictCallPolicy ect);

    ControlHandlerBuilder& with_dedicated_thread_pool(bool value);

    ControlHandlerBuilder& with_async_placement(bool value);
};

#endif //MONARCH_CONTROL_HANDLER_BUILDER_H
