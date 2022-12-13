//
// Created by dantas on 17/07/21.
//

#ifndef MONARCH_LIBC_OPERATION_HEADERS_H
#define MONARCH_LIBC_OPERATION_HEADERS_H

#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sstream>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#ifdef __linux__
#include <sys/vfs.h>
#elif __APPLE__
#include <sys/mount.h>
#endif

/**
* Metadata calls.
*/
typedef int (*libc_open_variadic_t) (const char*, int, ...);
typedef int (*libc_open_t) (const char*, int);
typedef int (*libc_openat_variadic_t) (int, const char*, int, ...);
typedef int (*libc_openat_t) (int, const char*, int);
typedef int (*libc_open64_variadic_t) (const char*, int, ...);
typedef int (*libc_open64_t) (const char*, int);
typedef int (*libc_close_t) (int);


/**
* libc_metadata struct: provides an object with the function pointers to all libc metadata-like
* operations.
*/
struct libc_metadata {
    libc_open_variadic_t m_open_var { (libc_open_variadic_t) dlsym(RTLD_NEXT, "open") };
    libc_open_t m_open { (libc_open_t) dlsym(RTLD_NEXT, "open") };
    libc_open64_variadic_t m_open64_var { (libc_open64_variadic_t) dlsym(RTLD_NEXT, "open64") };
    libc_open64_t m_open64 { (libc_open64_t) dlsym(RTLD_NEXT, "open64") };
    libc_close_t m_close { (libc_close_t) dlsym(RTLD_NEXT, "close") };
};

/**
* Data calls.
*/
typedef ssize_t (*libc_pread_t) (int, void*, size_t, off_t);
typedef void* (*libc_mmap_t) (void*, size_t, int, int, int, off_t);
#if defined(__USE_LARGEFILE64)
typedef ssize_t (*libc_pread64_t) (int, void*, size_t, off64_t);
#endif

/**
* libc_data: provides an object with the function pointers to all libc data-like operations.
*/
struct libc_data {
    libc_pread_t m_pread { (libc_pread_t) dlsym(RTLD_NEXT, "pread") };
    libc_mmap_t m_mmap { (libc_mmap_t) dlsym(RTLD_NEXT, "mmap") };
#if defined(__USE_LARGEFILE64)
    libc_pread64_t m_pread64 { (libc_pread64_t) dlsym(RTLD_NEXT, "pread64") };
#endif
};

#endif //MONARCH_LIBC_OPERATION_HEADERS_H
