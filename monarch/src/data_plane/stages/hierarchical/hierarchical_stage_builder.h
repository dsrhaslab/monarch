//
// Created by dantas on 25/10/20.
//

#ifndef MONARCH_HIERARCHICAL_STAGE_BUILDER_H
#define MONARCH_HIERARCHICAL_STAGE_BUILDER_H

#include "hierarchical_stage.h"


class HierarchicalStageBuilder {
    HierarchicalStage* hierarchical_stage;

public:
    explicit HierarchicalStageBuilder(int hierarchy_size){
        hierarchical_stage = new HierarchicalStage(hierarchy_size);
    };

    operator HierarchicalStage*() const {
        return hierarchical_stage;
    }

    HierarchicalStage* build(){
        if (hierarchical_stage->n_used_threads == 0){
            hierarchical_stage->n_used_threads = 1;
            hierarchical_stage->shared_thread_pool = true;
            hierarchical_stage->storage_hierarchy_thread_pools.push_back(new ctpl::thread_pool(1));
        }
        return hierarchical_stage;
    }

    HierarchicalStageBuilder& with_storage_hierarchy(std::vector<StorageDriver*> hierarchy){
        hierarchical_stage->storage_hierarchy = std::move(hierarchy);
        return *this;
    }

    HierarchicalStageBuilder& with_storage_hierarchy_pools(std::vector<ctpl::thread_pool*> pools){
        hierarchical_stage->storage_hierarchy_thread_pools = std::move(pools);
        return *this;
    }

    HierarchicalStageBuilder& with_private_debug_disabled(){
        hierarchical_stage->private_debug_enabled = false;
        return *this;
    }

    HierarchicalStageBuilder& with_shared_thread_pool(int pool_size){
        hierarchical_stage->shared_thread_pool = true;
        hierarchical_stage->n_used_threads = pool_size;
        hierarchical_stage->storage_hierarchy_thread_pools.push_back(new ctpl::thread_pool(pool_size));
        return *this;
    }
};

#endif //MONARCH_HIERARCHICAL_STAGE_BUILDER_H
