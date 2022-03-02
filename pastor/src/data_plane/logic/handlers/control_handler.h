//
// Created by dantas on 20/05/21.
//

#ifndef THESIS_CONTROL_HANDLER_H
#define THESIS_CONTROL_HANDLER_H

#include <unistd.h>

#include "../policies.h"
#include "../root/storage_drivers/utils/status.h"
#if defined BAZEL_BUILD || defined TF_BAZEL_BUILD
#include "third_party/ctpl/ctpl.h"
#else
#include <ctpl.h>
#endif

class PlacementHandler;
class EvictionHandler;
class HierarchicalDataPlane;
class PrefetchedFile;
class StrictFileInfo;
class File;
class PlacedFileInfo;
class ControlHandlerBuilder;

class ControlHandler {
private:
    ControlPolicy control_policy;
    PlacementPolicy placement_policy;
    EvictCallType evict_call_type;
    bool uses_dedicated_thread_pool;
    bool async_placement;

protected:
    ctpl::thread_pool* housekeeper_thread_pool;
    HierarchicalDataPlane* data_plane_;
    PlacementHandler* placement_handler;
    EvictionHandler* eviction_handler;

private:
    void base_evict(StrictFileInfo* mfi);
public:
    friend class ControlHandlerBuilder;
    explicit ControlHandler();
    virtual Status<ssize_t> place(File* f);
    virtual Status<ssize_t> place(PrefetchedFile* pf);
    virtual bool check_placement_validity();
    virtual bool check_placement_validity(PlacedFileInfo* f);
    virtual void evict(StrictFileInfo* mfi);
    virtual void prepare_environment();
    PlacementPolicy get_placement_policy();
    ControlPolicy get_control_policy();
    bool uses_async_calls() const;
    void change_policies(ControlPolicy cp, PlacementPolicy pp);
};

#endif //THESIS_CONTROL_HANDLER_H
