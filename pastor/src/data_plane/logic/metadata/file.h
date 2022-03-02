//
// Created by dantas on 19/10/20.
//

#ifndef THESIS_FILE_H
#define THESIS_FILE_H

#include <string>
#include "file_info.h"

class DataStorageDriver;

class File{
private:
    //int n_reads;
    char* content;
    uint64_t offset;
    size_t requested_size;
    bool full_read;

protected:
    FileInfo* info;

public:
    File(FileInfo* fi, uint64_t offset, size_t n);
    File(FileInfo* file_info);
    File(File* file);
    virtual ~File();

    [[nodiscard]] const std::string &get_filename() const;
    [[nodiscard]] char *get_content() const;
    void set_content(char *buffer);
    [[nodiscard]] size_t get_requested_size() const;
    virtual FileInfo* get_info();
    void print(int start, int finish);
    void reshape_to_full_request();
    size_t get_offset();
    bool is_full_read();
};


#endif //THESIS_FILE_H
