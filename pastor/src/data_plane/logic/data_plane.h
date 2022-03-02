//
// Created by dantas on 19/10/20.
//

#ifndef READ_CONTROLLER_H
#define READ_CONTROLLER_H

#include <unistd.h>
#include <string>
#include <tuple>

#include "absl/strings/string_view.h"

class CollectedStats;

class DataPlane {
public:
    virtual absl::string_view decode_filename(absl::string_view full_path) = 0;
    virtual int open(const char *pathname, int flags, mode_t mode, bool _64_option) = 0;
    virtual int open(const char *pathname, int flags, bool _64_option) = 0;
    virtual ssize_t pread(int fildes, char *result, size_t nbyte, uint64_t offset, bool _64_option) = 0;
    virtual int close(int fildes) = 0;
    virtual void *mmap (void *addr, size_t length, int prot, int flags, int fd, off_t offset) = 0;
    virtual ssize_t read(const std::string &filename, char* result, uint64_t offset, size_t n) = 0;
    virtual size_t get_file_size(const std::string &filename) = 0;
    virtual size_t get_file_size_from_id(int id) = 0;
    virtual int get_target_class(const std::string &filename) = 0;
    virtual ssize_t read_from_id(int file_id, char* result, uint64_t offset, size_t n) = 0;
    virtual ssize_t read_from_id(int file_id, char* result) = 0;
    virtual int get_target_class_from_id(int id) = 0;
    virtual void init(bool transparent_api) = 0;
    virtual void start() = 0;
    virtual void print() = 0;
    virtual int get_file_count() = 0;
    virtual void set_distributed_params(int rank, int worker_id) = 0;
    virtual ~DataPlane()= default;
};

#endif // READ_CONTROLLER_H
