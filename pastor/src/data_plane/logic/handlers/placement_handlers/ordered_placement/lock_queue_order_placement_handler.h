//
// Created by dantas on 19/05/21.
//

#ifndef THESIS_QUEUE_MANAGED_PREFETCH_DATA_PLANE_H
#define THESIS_QUEUE_MANAGED_PREFETCH_DATA_PLANE_H

#include <vector>
#include <queue>

#include "ordered_placement_handler.h"

class LockQueueOrderPlacementHandler : public OrderedPlacementHandler{
    std::priority_queue<PrefetchedFile*, std::vector<PrefetchedFile*>, earlierRequest> placement_queue;
    std::mutex mutex;
    Status<ssize_t> update_queue_and_place(std::unique_lock<std::mutex> ul);

    Status<ssize_t> inorder_place(PrefetchedFile* f);
public:
    LockQueueOrderPlacementHandler(HierarchicalDataPlane* data_plane, PlacementPolicy placement_policy, bool async_placement);
    Status<ssize_t> place(PrefetchedFile* f) override;
};

#endif //THESIS_QUEUE_MANAGED_PREFETCH_DATA_PLANE_H
