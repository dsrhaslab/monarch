//
// Created by dantas on 02/06/21.
//

#ifndef THESIS_ORDERED_PLACEMENT_HANDLER_H
#define THESIS_ORDERED_PLACEMENT_HANDLER_H

#include "../placement_handler.h"
#include "../../../metadata/prefetched_file.h"

class earlierRequest {
public:
    int operator() (const PrefetchedFile* pf1, const PrefetchedFile* pf2){
        return pf1->get_request_id() > pf2->get_request_id();
    }
};

class OrderedPlacementHandler : public PlacementHandler {
protected:
    int allocated_samples_index;

    bool in_order(PrefetchedFile* f) const {
        return f->get_request_id() == allocated_samples_index;
    };

public:
    OrderedPlacementHandler(HierarchicalDataPlane* data_plane, PlacementPolicy placement_policy, bool async_placement)
        : PlacementHandler(data_plane, placement_policy, async_placement) {
        allocated_samples_index = 0;
    }
};

#endif //THESIS_ORDERED_PLACEMENT_HANDLER_H
