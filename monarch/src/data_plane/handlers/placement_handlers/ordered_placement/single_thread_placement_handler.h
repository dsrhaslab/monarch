//
// Created by dantas on 27/05/21.
//

#ifndef THESIS_SINGLE_QUEUE_PLACEMENT_HANDLER_H
#define THESIS_SINGLE_QUEUE_PLACEMENT_HANDLER_H

#include <vector>
#include <queue>

#include "ordered_placement_handler.h"

class SingleThreadPlacementHandler : public OrderedPlacementHandler {
    std::priority_queue<PrefetchedFile*, std::vector<PrefetchedFile*>, earlierRequest> placement_queue;
    std::queue<PrefetchedFile*> postponed_queue;

    void flush_postponed();
    Status update_queue_and_place();
    Status inorder_place(PrefetchedFile* f);
public:
    friend class SingleThreadEvictionHandler;
    SingleThreadPlacementHandler(HierarchicalDataPlane* data_plane, PlacementPolicy placement_policy);
    Status place(PrefetchedFile* f) override;
};


#endif //THESIS_SINGLE_QUEUE_PLACEMENT_HANDLER_H
