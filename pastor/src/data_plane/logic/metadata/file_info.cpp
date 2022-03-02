//
// Created by dantas on 19/10/20.
//

#include <utility>
#include <cstring>
#include <iostream>
#include <memory>
#include "file_info.h"

FileInfo::FileInfo(std::string f, size_t file_size, int origin_level, bool has_shareable_file_descriptors)
    : mutex(std::make_unique<std::mutex>()) {

    filename = std::move(f);
    size = file_size;
    storage_level = origin_level;
    shareable_file_descriptors = has_shareable_file_descriptors;
    if(shareable_file_descriptors){
        for(int i = 0; i <= origin_level; i++){
            descriptors.emplace_back(-1, 0, 0);
        }
    }
    last_storage_level_read = origin_level;
}

FileInfo::FileInfo(std::string f, size_t s, int origin_level, bool has_shareable_file_descriptors, int t) : FileInfo(std::move(f),s,origin_level,has_shareable_file_descriptors) {
    target = t;
}

const std::string &FileInfo::get_name() const {
    return filename;
}

size_t FileInfo::_get_size() const {
    return size;
}

int FileInfo::get_storage_level() {
    return storage_level;
}

void FileInfo::loaded_to(int level) {
    storage_level = level;
}

int FileInfo::get_target(){
    return target;
}
void FileInfo::set_target(int t){
    target = t;
}

bool FileInfo::has_shareable_file_descriptors(){
    return shareable_file_descriptors;
};

std::unique_ptr<std::mutex> &FileInfo::get_mutex(){
    return mutex;
}

bool FileInfo::storage_changed(){
    return last_storage_level_read != storage_level;

}

int FileInfo::get_last_storage_read() const{
    return last_storage_level_read;
}

int FileInfo::update_last_storage_read(){
    last_storage_level_read = storage_level;
    return storage_level;
}

int FileInfo::get_file_descriptor(int level){
    return std::get<0>(descriptors[level]);
}

std::tuple<int, int, int>& FileInfo::get_descriptor_info(int level){
    return descriptors[level];
}

std::tuple<int, int, int>& FileInfo::get_descriptor_info_lsr(){
    return descriptors[last_storage_level_read];
}
