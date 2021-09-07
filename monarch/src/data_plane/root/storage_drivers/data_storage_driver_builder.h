//
// Created by dantas on 04/11/20.
//

#ifndef THESIS_DATA_STORAGE_DRIVER_BUILDER_H
#define THESIS_DATA_STORAGE_DRIVER_BUILDER_H

#include "data_storage_driver.h"
#include "file_system_driver.h"
#include "tbb_memory_buffer_driver.h"
#include "ph_memory_buffer_driver.h"

enum DriverType : unsigned int {
    FILE_SYSTEM = 1,
    TBB_MEMORY_BUFFER = 2,
    PH_MEMORY_BUFFER = 3,
};

class DataStorageDriverBuilder {
    BaseDataStorageDriver* data_storage_driver;
    size_t max_storage_size = 0;
    float threshold = 1;
    bool blocking_ = true;

public:
    explicit DataStorageDriverBuilder (DriverType dt) {
        switch (dt) {
            case FILE_SYSTEM:
                data_storage_driver = new FileSystemDriver();
                break;
            case TBB_MEMORY_BUFFER:
                data_storage_driver = new TBBMemoryBufferDriver();
                break;
            case PH_MEMORY_BUFFER:
                data_storage_driver = new PHMemoryBufferDriver();
                break;
        }
    };

    operator DataStorageDriver*() {return build();}

    DataStorageDriver* build();
    DataStorageDriverBuilder& with_allocation_capabilities(size_t max_storage_size, float threshold, bool blocking);
    DataStorageDriverBuilder& with_allocation_capabilities(size_t max_storage_size, bool blocking);
    DataStorageDriverBuilder& with_hierarchy_level(int level);
    DataStorageDriverBuilder& with_block_size(size_t block_size);
    DataStorageDriverBuilder& with_storage_prefix(const std::string& prefix);

};

#endif //THESIS_DATA_STORAGE_DRIVER_BUILDER_H
