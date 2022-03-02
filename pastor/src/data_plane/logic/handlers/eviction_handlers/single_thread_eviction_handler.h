//
// Created by dantas on 20/05/21.
//

#ifndef THESIS_SINGLE_THREAD_EVICTION_HANDLER_H
#define THESIS_SINGLE_THREAD_EVICTION_HANDLER_H

#include <queue>
#include "eviction_handler.h"
#include "absl/container/flat_hash_map.h"

#if defined BAZEL_BUILD || defined TF_BAZEL_BUILD
#include "third_party/ctpl/ctpl.h"
#else
#include <ctpl.h>
#endif

class PrefetchedFile;
class SingleThreadPlacementHandler;

class StatefulTransfer {
    StrictFileInfo* sfi_;
    int from_;
    int to_;

public:
    StatefulTransfer(StrictFileInfo* sfi, int from, int to) : sfi_(sfi), from_(from), to_(to) {}
};

class SingleThreadEvictionHandler : public EvictionHandler {
    SingleThreadPlacementHandler* placement_handler_;
    ctpl::thread_pool* housekeeper_thread_pool_;

private:
    void flush_postponed();
    void persist_changes(StrictFileInfo* mfi_, int from, int target) override;

public:
    SingleThreadEvictionHandler(HierarchicalDataPlane* data_plane, SingleThreadPlacementHandler* placement_handler, ctpl::thread_pool* housekeeper_thread_pool);
    void evict(StrictFileInfo* mfi) override;
};


#endif //THESIS_SINGLE_THREAD_EVICTION_HANDLER_H
