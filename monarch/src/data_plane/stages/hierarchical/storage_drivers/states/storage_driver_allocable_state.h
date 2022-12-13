//
// Created by dantas on 19/10/20.
//

#ifndef MONARCH_STORAGE_DRIVER_ALLOCABLE_STATE_H
#define MONARCH_STORAGE_DRIVER_ALLOCABLE_STATE_H

#include "storage_driver_state.h"

class StorageDriverBuilder;
class StorageDriverAllocableState : public StorageDriverState {
protected:
    size_t max_storage_size;
    size_t current_size;
    friend class StorageDriverBuilder;

    StorageDriverAllocableState(){
        max_storage_size = 0;
        current_size = 0;
    }

    explicit StorageDriverAllocableState(size_t n_max_storage_size){
        max_storage_size = n_max_storage_size;
        current_size = 0;
    }

public:
    Status<ssize_t> allocate_storage(Info* i) override;

    Status<ssize_t> free_storage(Info* i) override;

    bool becomes_full(Info* i) override;

    size_t resize(size_t new_size) override;

    size_t get_current_storage_size() override;

    size_t get_max_storage_size() const;

    StorageDriverStateType get_type() override;

    std::vector<std::string> configs() override;
};

#endif //MONARCH_STORAGE_DRIVER_ALLOCABLE_STATE_H