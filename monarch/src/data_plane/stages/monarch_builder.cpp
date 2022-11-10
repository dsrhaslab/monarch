//
// Created by dantas on 16/09/22.
//

#include "monarch_builder.h"

MonarchBuilder::MonarchBuilder(){
    monarch = new Monarch();
}

Monarch * MonarchBuilder::build() const {
    return monarch;
}

MonarchBuilder& MonarchBuilder::with_control_handler(ControlHandler* ch){
    monarch->control_handler = ch;
    return *this;
}

MonarchBuilder& MonarchBuilder::with_hierarchical_stage(HierarchicalStage* hs){
    monarch->hierarchical_stage = hs;
    return *this;
}

MonarchBuilder& MonarchBuilder::with_metadata_container(MetadataContainerService* mdc){
    monarch->metadata_container = mdc;
    return *this;
}

MonarchBuilder& MonarchBuilder::with_profiling_service(ProfilingService* pf){
    monarch->profiling_service = pf;
    return *this;
}

MonarchBuilder& MonarchBuilder::with_private_debug_disabled(){
    monarch->private_debug_enabled = false;
    return *this;
}