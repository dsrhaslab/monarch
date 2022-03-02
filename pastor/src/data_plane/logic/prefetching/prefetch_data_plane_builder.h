//
// Created by dantas on 29/10/20.
//

#ifndef THESIS_PREFETCH_DATA_PLANE_BUILDER_H
#define THESIS_PREFETCH_DATA_PLANE_BUILDER_H

#include "prefetch_data_plane.h"

class PrefetchDataPlaneBuilder  {
    PrefetchDataPlane* data_plane;
    ControlPolicy control_policy;
    ReadPolicy read_policy;
    PlacementPolicy placement_policy;
    EvictCallType evict_call_type;

public:
    explicit PrefetchDataPlaneBuilder(HierarchicalDataPlane* root_data_plane){
        data_plane = new PrefetchDataPlane(root_data_plane);
        control_policy = LOCK_ORDERED;
        read_policy = WAIT_ENFORCED;
        placement_policy = FIRST_LEVEL_ONLY;
        evict_call_type = CLIENT;
    };

    DataPlane* build();
    PrefetchDataPlaneBuilder& with_eviction_percentage(float percentage);
    PrefetchDataPlaneBuilder& with_prefetch_thread_pool_size(int size);
    PrefetchDataPlaneBuilder& with_read_policy(ReadPolicy rp);
    PrefetchDataPlaneBuilder& configurations();
    PrefetchDataPlaneBuilder& policies();
};


#endif //THESIS_PREFETCH_DATA_PLANE_BUILDER_H
