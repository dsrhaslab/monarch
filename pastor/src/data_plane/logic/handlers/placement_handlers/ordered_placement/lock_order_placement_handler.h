//
// Created by dantas on 19/05/21.
//

#ifndef THESIS_BLOCKING_PREFETCH_DATA_PLANE_H
#define THESIS_BLOCKING_PREFETCH_DATA_PLANE_H

#include "ordered_placement_handler.h"
#include <mutex>
#include <condition_variable>

class LockOrderPlacementHandler : public OrderedPlacementHandler {
    std::condition_variable in_order_condition;
    std::mutex mutex;
public:
    LockOrderPlacementHandler(HierarchicalDataPlane* data_plane, PlacementPolicy placement_policy, bool async_placement);
    Status<ssize_t> place(PrefetchedFile* f) override;
};


#endif //THESIS_BLOCKING_PREFETCH_DATA_PLANE_H
