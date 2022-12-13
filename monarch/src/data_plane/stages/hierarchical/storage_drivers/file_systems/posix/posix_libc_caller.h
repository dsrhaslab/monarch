//
// Created by dantas on 26/09/22.
//

#ifndef MONARCH_POSIX_LIBC_CALLER_H
#define MONARCH_POSIX_LIBC_CALLER_H

#include <string>
#include <dlfcn.h>
#include <mutex>

#include "libc_operation_headers.h"
#include "libc_operations_enums.h"

class PosixLibcCaller {
    static std::string m_lib_name; 
    static void* m_lib_handle;
    static libc_metadata m_metadata_operations;
    static libc_data m_data_operations;
    static bool init_done;
    static std::mutex mutex_;

    static bool dlopen_library_handle ();

public:
    static void init();

    static int open(const char *pathname, int flags, mode_t mode);

    static int open(const char *pathname, int flags);

    static int open64(const char *pathname, int flags, mode_t mode);

    static int open64(const char *pathname, int flags);

    static ssize_t pread(int fildes, void *result, size_t nbyte, off_t offset);

    static ssize_t pread64(int fildes, void *result, size_t nbyte, off_t offset);

    static void *mmap (void *addr, size_t length, int prot, int flags, int fd, off_t offset);

    static int close(int fildes);
};

#endif // MONARCH_POSIX_LIBC_CALLER_H