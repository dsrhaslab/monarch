//
// Created by dantas on 28/10/20.
//

#include "strict_file_info.h"
#include <memory>
#include <utility>
#include <iostream>

StrictFileInfo::StrictFileInfo(std::string f, size_t size, int origin_level, bool has_shareable_file_descriptors) : FileInfo(std::move(f), size, origin_level, has_shareable_file_descriptors) {
    unstable = false;
    n_reads = 0;
    wait_on_loaded = 0;
    wait_on_unstable = 0;
    staged_level = origin_level;
}

StrictFileInfo::StrictFileInfo(std::string f, size_t size, int origin_level, bool has_shareable_file_descriptors, int target) : FileInfo(std::move(f), size, origin_level, has_shareable_file_descriptors, target) {
    unstable = false;
    n_reads = 0;
    wait_on_loaded = 0;
    wait_on_unstable = 0;
    staged_level = origin_level;
}

void StrictFileInfo::moved_to(int level) {
    std::unique_lock<std::mutex> ul(mutex);
    unstable = false;
    staged_level = level;
    FileInfo::loaded_to(level);
    ul.unlock();
    if(wait_on_unstable > 0)
        unstable_condition.notify_all();
}

void StrictFileInfo::loaded_to(int level) {
    std::unique_lock<std::mutex> ul(mutex);
    unstable = false;
    FileInfo::loaded_to(level);
    ul.unlock();
    if(level == 0 && wait_on_loaded > 0)
        loaded_condition.notify_all();
    if(wait_on_unstable > 0)
        unstable_condition.notify_all();
}

//true leads to a placeholder
bool StrictFileInfo::init_prefetch(){
    bool waited = false;
    std::unique_lock<std::mutex> ul(mutex);
    while(unstable) {
        if(!waited) {
            wait_on_unstable++;
        }
        waited = true;
        unstable_condition.wait(ul);
    }
    if(waited)
        wait_on_unstable--;
    return ++n_reads == 1 && storage_level != 0;
}


bool StrictFileInfo::await_loaded_data(int target_level) {
    bool waited = false;
    std::unique_lock<std::mutex> ul(mutex);
    while (storage_level != target_level) {
        if(!waited) {
            wait_on_loaded++;
        }
        waited = true;
        loaded_condition.wait(ul);
    }
    if(waited)
        wait_on_loaded--;
    return waited;
}


int StrictFileInfo::finish_read(){
    std::unique_lock<std::mutex> ul(mutex);
    int res = --n_reads;
    if(res == 0)
        unstable = true;
    return res;
}


int StrictFileInfo::get_staging_level() const{
    return staged_level;
}