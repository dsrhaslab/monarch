//
// Created by dantas on 19/10/20.
//

#include "file.h"
#include <cstring>
#include <iostream>

File::File(FileInfo* fi, uint64_t offset, size_t n){
    File::info = fi;
    File::offset = offset;
    File::requested_size = n + offset > fi->_get_size() ? fi->_get_size() - offset : n;
    File::content = new char[n];
    File::full_read = requested_size == fi->_get_size();
}

File::File(FileInfo* file_info){
    File::info = file_info;
    File::requested_size = info->_get_size();
    File::offset = 0;
    File::content = new char[requested_size];
    File::full_read = requested_size == file_info->_get_size();
}

File::File(File* file){
    File::info = file->info;
    File::requested_size = file->requested_size;
    File::offset = file->offset;
    File::content = file->content;
    File::full_read = file->full_read;
}

File::~File() {
    delete[] content;
}

void File::reshape_to_full_request(){
    delete[] content;
    File::requested_size = info->_get_size();
    File::offset = 0;
    File::content = new char[requested_size];
}

const std::string &File::get_filename() const {
    return info->get_name();
}


char *File::get_content() const {
    return content;
}

void File::set_content(char *buffer) {
    File::content = buffer;
}

size_t File::get_requested_size() const{
    return requested_size;
}

FileInfo* File::get_info(){
    return info;
}

size_t File::get_offset(){
    return offset;
}

bool File::is_full_read(){
    return full_read;
}

void File::print(int start, int finish){
    for (int i = start; i < finish; i++)
        std::cout << content[i];
    std::cout << "\n";
}