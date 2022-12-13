//
// Created by dantas on 20/05/21.
//

#ifndef MONARCH_CONTROL_HANDLER_H
#define MONARCH_CONTROL_HANDLER_H

#include <unistd.h>

#if defined BAZEL_BUILD || defined TF_BAZEL_BUILD
#include "third_party/ctpl/ctpl.h"
#else
#include "ctpl.h"
#endif

#include "../types.h"
#include "../utils/status.h"

class PlacementHandler;
class Monarch;
class File;
class Info;
class ControlHandlerBuilder;

class ControlHandler {
private:
    ControlPolicy control_policy;
    PlacementPolicy placement_policy;
    bool uses_dedicated_thread_pool;
    bool async_placement;

    explicit ControlHandler();

protected:
    ctpl::thread_pool* housekeeper_thread_pool;
    Monarch* monarch_;
    PlacementHandler* placement_handler;
    //EvictionHandler* eviction_handler;

public:
    friend class ControlHandlerBuilder;
    static ControlHandlerBuilder create();
    Status<ssize_t> place(File* f);
    bool check_placement_validity();
    bool check_placement_validity(Info* i);
    //virtual void evict(Info* i);
    StorageDriverStateType prepare_environment(Monarch* monarch);
    PlacementPolicy get_placement_policy();
    ControlPolicy get_control_policy();
    bool uses_async_calls() const;
    void change_policies(ControlPolicy cp, PlacementPolicy pp);
    //Needs to be called by Monarch itself
    void set_monarch(Monarch* monarch);
};

#endif //MONARCH_CONTROL_HANDLER_H
