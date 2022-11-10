//
// Created by dantas on 19/10/20.
//

#ifndef MONARCH_STORAGE_DRIVER_BLOCKING_STATE_H
#define MONARCH_STORAGE_DRIVER_BLOCKING_STATE_H

#include <condition_variable>

#include "storage_driver_allocable_state.h"

class StorageDriverBlockingState : public StorageDriverAllocableState {
    int waiting;
    std::unique_ptr<std::mutex> mutex;
    std::unique_ptr<std::condition_variable> is_full;

public:
    explicit StorageDriverBlockingState(StorageDriverAllocableState* storage_state);

    std::vector<std::string> configs() override;

    Status<ssize_t> allocate_storage(Info* fi) override;

    Status<ssize_t> free_storage(Info* fi) override;

    bool becomes_full(Info* fi) override;

    size_t resize(size_t new_size) override;

    size_t get_current_storage_size() override;

    StorageDriverStateType get_type() override;
};

#endif //MONARCH_STORAGE_DRIVER_BLOCKING_STATE_H