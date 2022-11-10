//
// Created by dantas on 04/11/20.
//

#include "storage_driver_eventual_state.h"

StorageDriverEventualState::StorageDriverEventualState(StorageDriverAllocableState* storage_state)
{
    current_size = storage_state->get_current_storage_size();
    max_storage_size = storage_state->get_max_storage_size();
    placeholder_current_size = 0;
}

Status<ssize_t> StorageDriverEventualState::allocate_storage(Info* i){
    ssize_t bytes_to_alloc = i->size;
    if (bytes_to_alloc + placeholder_current_size > max_storage_size)
        return {STORAGE_FULL};

    placeholder_current_size += bytes_to_alloc;
    return {bytes_to_alloc};
}

Status<ssize_t> StorageDriverEventualState::free_storage(Info* i){
    ssize_t bytes_to_alloc = i->size;
    placeholder_current_size -= bytes_to_alloc;
    return {bytes_to_alloc};
}

size_t StorageDriverEventualState::get_current_storage_size(){
    return placeholder_current_size;
}

bool StorageDriverEventualState::becomes_full(Info* i){
    return i->size + placeholder_current_size >= max_storage_size;
}

StorageDriverStateType StorageDriverEventualState::get_type(){
    return StorageDriverStateType::EVENTUAL;
}

std::vector<std::string> StorageDriverEventualState::configs(){
    auto configs = StorageDriverAllocableState::configs();
    configs.push_back("Eventual: true\n");
    return configs;
}
