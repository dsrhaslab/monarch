//
// Created by dantas on 04/11/20.
//

#include "storage_driver_blocking_state.h"

StorageDriverBlockingState::StorageDriverBlockingState(StorageDriverAllocableState* storage_state)
{
    current_size = storage_state->get_current_storage_size();
    max_storage_size = storage_state->get_max_storage_size();
    mutex = std::make_unique<std::mutex>();
    is_full = std::make_unique<std::condition_variable>();
    waiting = 0;
}

Status<ssize_t> StorageDriverBlockingState::allocate_storage(Info* i){
    ssize_t bytes_to_alloc = i->size;
    std::unique_lock<std::mutex> ul(*mutex);
    bool waited = false;
    while(bytes_to_alloc + current_size > max_storage_size) {
        //for spurious wakeup
        if(!waited)
            waiting++;
        waited = true;
        is_full->wait(ul);
    }
    if(waited)
        waiting--;
    current_size += bytes_to_alloc;
    return {bytes_to_alloc};
}

Status<ssize_t> StorageDriverBlockingState::free_storage(Info* i){
    ssize_t bytes_to_alloc = i->size;
    bool notify = false;
    {
        std::lock_guard<std::mutex> ul(*mutex);
        current_size -= bytes_to_alloc;
        notify = waiting > 0;
    }
    if(notify)
        is_full->notify_all();
    return {bytes_to_alloc};
}

bool StorageDriverBlockingState::becomes_full(Info* i){
    std::lock_guard<std::mutex> ul(*mutex);
    return i->size + current_size >= max_storage_size;
}

size_t StorageDriverBlockingState::resize(size_t new_size){
    std::lock_guard<std::mutex> ul(*mutex);
    max_storage_size = new_size;
    return new_size;
}

size_t StorageDriverBlockingState::get_current_storage_size(){
    std::lock_guard<std::mutex> ul(*mutex);
    return current_size;
}

StorageDriverStateType StorageDriverBlockingState::get_type() {
    return StorageDriverStateType::BLOCKING;
}

std::vector<std::string> StorageDriverBlockingState::configs() {
    auto configs = StorageDriverAllocableState::configs();
    configs.push_back("Blocking: true\n");
    return configs;
}
