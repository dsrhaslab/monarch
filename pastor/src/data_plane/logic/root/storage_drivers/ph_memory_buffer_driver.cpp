//
// Created by dantas on 24/05/21.
//

#include "ph_memory_buffer_driver.h"

Status<ssize_t> PHMemoryBufferDriver::read(FileInfo* fi, char* result, uint64_t offset, size_t n, bool _64_option){
    File* fc = buffer[fi->get_name()];
    memcpy(result, fc->get_content() + offset, n);
    return {static_cast<ssize_t>(n)};
}

Status<ssize_t> PHMemoryBufferDriver::read(File* f){
    return read(f->get_info(), f->get_content(), f->get_offset(), f->get_requested_size(), false);
}

Status<ssize_t> PHMemoryBufferDriver::write(File* f) {
    buffer[f->get_filename()] = f;
    return {SUCCESS};
}


Status<ssize_t> PHMemoryBufferDriver::remove(FileInfo* fi){
    File* f = buffer[fi->get_name()];
    buffer.erase(fi->get_name());
    delete f;
    return {SUCCESS};
}

ssize_t PHMemoryBufferDriver::sizeof_content(FileInfo* fi) {
    return fi->_get_size() + fi->get_name().size();
}

bool PHMemoryBufferDriver::in_memory_type(){
   return true;
}

bool PHMemoryBufferDriver::file_system_type(){
    return false;
}

File* PHMemoryBufferDriver::remove_for_copy(FileInfo* fi){
    File* f = buffer[fi->get_name()];
    buffer.erase(fi->get_name());
    return f;
}

void PHMemoryBufferDriver::create_environment(std::vector<std::string>& dirs, bool enable_write){}