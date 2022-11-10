//
// Created by dantas on 01/11/20.
//

#include <cstring>
#include "tbb_memory_buffer_driver.h"

Status<ssize_t> TBBMemoryBufferDriver::read(Info* i, char* result, size_t n, off_t offset){
    content_hash_map::const_accessor a;
    bool file_in_buffer = buffer.find(a, i->name);
    if(file_in_buffer) {
        File* fc = a->second;

        // Release lock for this filename
        a.release();
        // TODO see if this copy can be removed
        memcpy(result, fc->content + offset, n);
        return {static_cast<ssize_t>(n)};
    }
    else {
        // Release lock for this filename
        a.release();

        return {NOT_FOUND};
    }
}


Status<ssize_t> TBBMemoryBufferDriver::write(File* f){
    // Check if buffer contains file
    content_hash_map::accessor a;
    bool new_file = buffer.insert(a, f->info->name);

    if (!new_file) {
        // Release lock for this filename
        a.release();
    } else {
        // Add file with no content to buffer
        a->second = f;

        // Release lock for this filename
        a.release();
        return {f->info->size};
    }
    return {SUCCESS};
}


//TODO warning: deleting object of polymorphic class type 'File' which has non-virtual destructor might cause undefined behavior
Status<ssize_t> TBBMemoryBufferDriver::remove(Info* i){
    content_hash_map::const_accessor a;
    bool file_in_buffer = buffer.find(a, i->name);
    if(file_in_buffer) {
        File* f = a->second;
        // Release lock for this filename
        a.release();
        bool res = buffer.erase(i->name);
        delete f;
        if (res)
            return {SUCCESS};
        else
            return {NOT_FOUND};
    }
    return {NOT_FOUND};
}

StorageDriverSubType TBBMemoryBufferDriver::get_subtype(){
    return StorageDriverSubType::THREAD_BUILDING_BLOCKS;
}

File* TBBMemoryBufferDriver::remove_for_copy(Info* i){
    content_hash_map::const_accessor a;
    bool file_in_buffer = buffer.find(a, i->name);
    if(file_in_buffer) {
        File *f = a->second;
        // Release lock for this filename
        a.release();
        buffer.erase(i->name);
        return f;
    }
    else {
        std::cerr << "remove_for_copy failed for file: " << i->name;
        exit(1);
    }
}