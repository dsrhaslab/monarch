//
// Created by dantas on 19/10/20.
//

#ifndef MONARCH_STORAGE_DRIVER_EVENTUAL_STATE_H
#define MONARCH_STORAGE_DRIVER_EVENTUAL_STATE_H

#include "storage_driver_allocable_state.h"

//@deprecated

class StorageDriverEventualState : public StorageDriverAllocableState {
    size_t placeholder_current_size;

public:
    explicit StorageDriverEventualState(StorageDriverAllocableState* storage_state);

    Status<ssize_t> allocate_storage(Info* fi) override;

    Status<ssize_t> free_storage(Info* fi) override;

    size_t get_current_storage_size() override;

    bool becomes_full(Info* fi) override;

    StorageDriverStateType get_type() override;

    std::vector<std::string> configs() override;
};

#endif //MONARCH_STORAGE_DRIVER_EVENTUAL_STATE_H