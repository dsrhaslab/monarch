//
// Created by dantas on 24/05/21.
//

#include "ph_memory_buffer_driver.h"

Status<ssize_t> PHMemoryBufferDriver::read(Info* i, char* result, size_t n, off_t offset){
    File* fc = buffer[i->name];
    memcpy(result, fc->content + offset, n);
    return {static_cast<ssize_t>(n)};
}

Status<ssize_t> PHMemoryBufferDriver::write(File* f) {
    buffer[f->info->name] = f;
    return {SUCCESS};
}

Status<ssize_t> PHMemoryBufferDriver::remove(Info* i){
    File* f = buffer[i->name];
    buffer.erase(i->name);
    delete f;
    return {SUCCESS};
}

StorageDriverSubType PHMemoryBufferDriver::get_subtype(){
    return StorageDriverSubType::PARALLEL_HASHMAP;
}

File* PHMemoryBufferDriver::remove_for_copy(Info* i){
    File* f = buffer[i->name];
    buffer.erase(i->name);
    return f;
}
