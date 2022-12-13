//
// Created by dantas on 04/11/22.
//

#ifndef MONARCH_MEMORY_BUFFER_DRIVER_BUILDER_H
#define MONARCH_MEMORY_BUFFER_DRIVER_BUILDER_H

#include "../storage_driver_builder.h"
#include "ph_memory_buffer_driver.h"
#include "tbb_memory_buffer_driver.h"

class MemoryBufferDriverBuilder : public StorageDriverBuilder {
public:
    explicit MemoryBufferDriverBuilder(StorageDriverSubType subtype){
        switch (subtype)
        {
        case StorageDriverSubType::THREAD_BUILDING_BLOCKS:
            base_storage_driver = new TBBMemoryBufferDriver();
            break;
        case StorageDriverSubType::PARALLEL_HASHMAP:
            base_storage_driver = new PHMemoryBufferDriver();
            break;
        default:
            break;
        }
    }
};

#endif //MONARCH_MEMORY_BUFFER_DRIVER_BUILDER_H