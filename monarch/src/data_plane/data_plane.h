//
// Created by dantas on 19/10/20.
//

#ifndef READ_CONTROLLER_H
#define READ_CONTROLLER_H

#include <unistd.h>
#include <string>
#include <tuple>

class CollectedStats;

class DataPlane {
public:
    virtual std::string decode_filename(std::string full_path) = 0;
    virtual ssize_t read(const std::string &filename, char* result, uint64_t offset, size_t n) = 0;
    virtual size_t get_file_size(const std::string &filename) = 0;
    virtual size_t get_file_size_from_id(int id) = 0;
    virtual int get_target_class(const std::string &filename) = 0;
    virtual ssize_t read_from_id(int file_id, char* result, uint64_t offset, size_t n) = 0;
    virtual ssize_t read_from_id(int file_id, char* result) = 0;
    virtual int get_target_class_from_id(int id) = 0;
    virtual void init() = 0;
    virtual void start() = 0;
    virtual void print() = 0;
    virtual int get_file_count() = 0;
    virtual CollectedStats* collect_statistics() = 0;
    virtual void set_distributed_params(int rank, int worker_id) = 0;
};

#endif // READ_CONTROLLER_H
