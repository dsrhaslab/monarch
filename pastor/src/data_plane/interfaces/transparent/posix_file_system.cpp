//
// Created by dantas on 17/07/21.
//
#include <fcntl.h>
#include <iostream>
#include <cstdarg>
#include <cstring>
#include "posix_file_system.h"

int open (const char* path, int flags, ...)
{
    if (flags & O_CREAT) {
        va_list args;

        va_start (args, flags);
        mode_t mode = va_arg (args, int);
        va_end (args);

        return monarch.open(path, flags, mode);
    } else {
        return monarch.open(path, flags);
    }
}

int open64 (const char* path, int flags, ...)
{
    if (flags & O_CREAT) {
        va_list args;

        va_start (args, flags);
        mode_t mode = va_arg (args, int);
        va_end (args);

        return monarch.open64(path, flags, mode);
    } else {
        return monarch.open64(path, flags);
    }
}

// pread call. (...)
ssize_t pread (int fd, void* buf, size_t size, off_t offset)
{
    return monarch.pread(fd, buf, size, offset);
}


// pread64 call. (...)
ssize_t pread64 (int fd, void* buf, size_t size, off_t offset)
{
    return monarch.pread64(fd, buf, size, offset);
}

// close call. (...)
int close (int fd)
{
    return monarch.close(fd);
}

void *mmap (void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
    return monarch.mmap(addr, length, prot, flags, fd, offset);
}

/*
int munmap(void *addr, size_t length)
{
    return monarch.munmap(addr, length);
}


int mprotect(void *addr, size_t len, int prot)
{
    return monarch.mprotect(addr, len, prot);
}
*/
