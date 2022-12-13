//
// Created by dantas on 16/09/22.
//

#ifndef MONARCH_MONARCH_BUILDER_H
#define MONARCH_MONARCH_BUILDER_H

#include "monarch.h"
#include "../handlers/control_handler_builder.h"

class MonarchBuilder {
    Monarch* monarch;

public:
    explicit MonarchBuilder();

    explicit operator Monarch*() const {return build();}

    Monarch * build() const;

    MonarchBuilder& with_control_handler(ControlHandler* chb);

    MonarchBuilder& with_hierarchical_stage(HierarchicalStage* hs);

    MonarchBuilder& with_metadata_container(MetadataContainerService* mdc);

    MonarchBuilder& with_profiling_service(ProfilingService* pf);

    MonarchBuilder& with_private_debug_disabled();
};

#endif //MONARCH_MONARCH_BUILDER_H