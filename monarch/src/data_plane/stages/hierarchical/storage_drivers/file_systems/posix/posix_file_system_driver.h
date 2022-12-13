//
// Created by dantas on 19/10/20.
//

#ifndef MONARCH_POSIX_FILE_SYSTEM_DRIVER_H
#define MONARCH_POSIX_FILE_SYSTEM_DRIVER_H

#include <atomic>

#include "../file_system_driver.h"
#include "libc_operation_headers.h"
#include "libc_operations_enums.h"
#include "posix_libc_caller.h"

class PosixFileSystemDriverBuilder;

class PosixFileSystemDriver : public FileSystemDriver {
    size_t block_size = INT32_MAX;

    PosixFileSystemDriver();

    std::basic_string<char> get_full_path(const std::string &filename);

    std::basic_string<char> get_full_path(const char* filename);

    ssize_t read_block(int fd, char *result, size_t n, off_t offset, bool _64_option);

    ssize_t read(int fd, char *result, size_t n, off_t offset, bool _64_option);

    ssize_t write_block(int fd, char *buf, size_t n, off_t offset);

    ssize_t write(int fd, char *buf, size_t n, off_t offset);

    void create_dir(const std::string &path);

    friend class PosixFileSystemDriverBuilder;

public:
    Status<ssize_t> read(Info *i, char *result, size_t n, off_t offset) override;

    Status<ssize_t> write(File *f) override;

    Status<ssize_t> remove(Info *i) override;

    StorageDriverSubType get_subtype() override;

    void create_environment(std::vector<std::string> &dirs, bool enable_write) override;

    int open_(const char *pathname, int flags, mode_t mode);

    int open_(const char *pathname, int flags);

    int open64_(const char *pathname, int flags, mode_t mode);

    int open64_(const char *pathname, int flags);

    ssize_t pread_(int fildes, void *result, size_t nbyte, off_t offset);

    ssize_t pread64_(int fildes, void *result, size_t nbyte, off_t offset);

    ssize_t pwrite_(int fildes, char *buf, size_t count, off_t offset);

    void *mmap_(void *addr, size_t length, int prot, int flags, int fd, off_t offset);

    int close_(int fildes);

    static PosixFileSystemDriverBuilder* create();
};

#endif // MONARCH_POSIX_FILE_SYSTEM_DRIVER_H
