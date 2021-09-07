//
// Created by dantas on 20/05/21.
//

#ifndef THESIS_SINGLE_THREAD_EVICTION_HANDLER_H
#define THESIS_SINGLE_THREAD_EVICTION_HANDLER_H

#include <queue>
#include "eviction_handler.h"
#include "absl/container/flat_hash_map.h"

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

private:
    void flush_postponed();
    void persist_changes(StrictFileInfo* mfi_, int from, int target) override;

public:
    SingleThreadEvictionHandler(HierarchicalDataPlane* data_plane, SingleThreadPlacementHandler* placement_handler);
    void evict(StrictFileInfo* mfi) override;
};


#endif //THESIS_SINGLE_THREAD_EVICTION_HANDLER_H
