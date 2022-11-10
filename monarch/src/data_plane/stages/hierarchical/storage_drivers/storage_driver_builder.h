//
// Created by dantas on 04/11/20.
//

#ifndef MONARCH_DATA_STORAGE_DRIVER_BUILDER_H
#define MONARCH_DATA_STORAGE_DRIVER_BUILDER_H

#include "storage_driver.h"
#include "states/storage_driver_allocable_state.h"

class StorageDriverBuilder {
    size_t max_storage_size_ = 0;

protected:
    StorageDriver* base_storage_driver = nullptr;

public:
    explicit operator StorageDriver*() {return build();}

    //TODO maybe this should be merged with the control_handler constructor logic
    StorageDriver* build(){
        if (max_storage_size_ > 0){
            base_storage_driver->state = new StorageDriverAllocableState(max_storage_size_);
        }
        return base_storage_driver;
    }   

    StorageDriverBuilder& with_allocation_capabilities(size_t max_storage_size){
        max_storage_size_ = max_storage_size;
        return *this;
    }
};

#endif //MONARCH_DATA_STORAGE_DRIVER_BUILDER_H
