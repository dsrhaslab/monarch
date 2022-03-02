//
// Created by dantas on 01/11/20.
//

#include <cstring>
#include "tbb_memory_buffer_driver.h"

Status<ssize_t> TBBMemoryBufferDriver::read(FileInfo* fi, char* result, uint64_t offset, size_t n, bool _64_option){
    content_hash_map::const_accessor a;
    bool file_in_buffer = buffer.find(a, fi->get_name());

    if(file_in_buffer) {
        File* fc = a->second;

        // Release lock for this filename
        a.release();

        // TODO see if this copy can be removed
        memcpy(result, fc->get_content() + offset, n);
        return {static_cast<ssize_t>(n)};
    }
    else {
        // Release lock for this filename
        a.release();

        return {NOT_FOUND};
    }
}

Status<ssize_t> TBBMemoryBufferDriver::read(File* f){
    return read(f->get_info(), f->get_content(), f->get_offset(), f->get_requested_size(), false);
}


//TODO warning: deleting object of polymorphic class type 'File' which has non-virtual destructor might cause undefined behavior
Status<ssize_t> TBBMemoryBufferDriver::remove(FileInfo* fi){
    content_hash_map::const_accessor a;
    bool file_in_buffer = buffer.find(a, fi->get_name());
    if(file_in_buffer) {
        File* f = a->second;
        // Release lock for this filename
        a.release();
        bool res = buffer.erase(fi->get_name());
        delete f;
        if (res)
            return {SUCCESS};
        else
            return {NOT_FOUND};
    }
    return {NOT_FOUND};
}

File* TBBMemoryBufferDriver::remove_for_copy(FileInfo* fi){
    content_hash_map::const_accessor a;
    bool file_in_buffer = buffer.find(a, fi->get_name());
    if(file_in_buffer) {
        File *f = a->second;
        // Release lock for this filename
        a.release();
        buffer.erase(fi->get_name());
        return f;
    }
    else {
        std::cerr << "remove_for_copy failed for file: " << fi->get_name();
        exit(1);
    }
}


Status<ssize_t> TBBMemoryBufferDriver::write(File* f){
    // Check if buffer contains file
    content_hash_map::accessor a;
    bool new_file = buffer.insert(a, f->get_filename());

    if (!new_file) {
        // Release lock for this filename
        a.release();
    } else {
        // Add file with no content to buffer
        a->second = f;

        // Release lock for this filename
        a.release();
        return {sizeof_content(f->get_info())};
    }
    return {SUCCESS};
}

ssize_t TBBMemoryBufferDriver::sizeof_content(FileInfo* fi){
    return fi->_get_size() + fi->get_name().size();
}

bool TBBMemoryBufferDriver::in_memory_type(){
    return true;
}

bool TBBMemoryBufferDriver::file_system_type(){
    return false;
}

void TBBMemoryBufferDriver::create_environment(std::vector<std::string>& dirs, bool enable_write){}