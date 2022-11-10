//
// Created by dantas on 19/10/20.
//

#ifndef MONARCH_FILE_H
#define MONARCH_FILE_H

#include <string>
#include "../metadata/info.h"

class StorageDriver;

struct File{
    //int n_reads;
    bool is_full_read;
    size_t requested_size;
    char* content;
    Info* info;
    off_t offset;

    File(Info* fi, off_t offset, size_t n){
        File::info = fi;
        File::offset = offset;
        File::requested_size = n + offset > fi->size ? fi->size - offset : n;
        File::content = new char[n];
        File::is_full_read = requested_size == fi->size;
    };

    File(Info* file_info){
        File::info = file_info;
        File::requested_size = info->size;
        File::offset = 0;
        File::content = new char[requested_size];
        File::is_full_read = requested_size == file_info->size;
    };

    File(File* file){
        File::info = file->info;
        File::requested_size = file->requested_size;
        File::offset = file->offset;
        File::content = file->content;
        File::is_full_read = file->is_full_read;
    };

    ~File() {
        delete[] content;
    }

    void reshape_to_full_request() {
        delete[] content;
        File::requested_size = info->size;
        File::offset = 0;
        File::content = new char[requested_size];
    };
};


#endif //MONARCH_FILE_H
