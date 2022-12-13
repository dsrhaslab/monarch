//
// Created by dantas on 04/11/20.
//

#include "storage_driver_allocable_state.h"

Status<ssize_t> StorageDriverAllocableState::allocate_storage(Info* i){
    ssize_t bytes_to_alloc = i->size;
    if (bytes_to_alloc + current_size > max_storage_size){
        return {STORAGE_FULL};
    }

    current_size += bytes_to_alloc;
    return {bytes_to_alloc};
}

Status<ssize_t> StorageDriverAllocableState::free_storage(Info* i){
    ssize_t bytes_to_alloc = i->size;
    current_size -= bytes_to_alloc;
    return {bytes_to_alloc};
}

bool StorageDriverAllocableState::becomes_full(Info* i){
    return i->size + current_size >= max_storage_size;
}

size_t StorageDriverAllocableState::resize(size_t new_size){
    max_storage_size = new_size;
    return new_size;
}

size_t StorageDriverAllocableState::get_current_storage_size(){
    return current_size;
}

size_t StorageDriverAllocableState::get_max_storage_size() const{
    return max_storage_size;
}

StorageDriverStateType StorageDriverAllocableState::get_type() {
    return StorageDriverStateType::ALLOCABLE;
}

//TODO
std::vector<std::string> StorageDriverAllocableState::configs(){

}