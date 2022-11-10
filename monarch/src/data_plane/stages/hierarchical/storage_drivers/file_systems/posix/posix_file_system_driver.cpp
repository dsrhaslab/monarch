//
// Created by dantas on 19/10/20.
//

#include <bits/stdc++.h>
#include <cstring>
#include <dlfcn.h>
#include <fcntl.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "absl/strings/str_cat.h"
#include "posix_file_system_driver.h"
#include "posix_file_system_driver_builder.h"

PosixFileSystemDriver::PosixFileSystemDriver(){
  PosixLibcCaller::init();
}

inline std::basic_string<char> PosixFileSystemDriver::get_full_path(const std::string &filename)
{
    return absl::StrCat(storage_prefix, filename);
}

inline std::basic_string<char> PosixFileSystemDriver::get_full_path(const char* filename)
{
    return absl::StrCat(storage_prefix, filename);
}

ssize_t PosixFileSystemDriver::read_block(int fd, char *result, size_t n, off_t off, bool _64_option)
{
  int error = 0;
  char *dst = result;
  ssize_t bytes_read = 0;

  while (n > 0 && error == 0)
  {
    // Some platforms, notably macs, throw EINVAL if pread is asked to read
    // more than fits in a 32-bit integer.
    size_t requested_read_length;
    if (n > INT32_MAX)
    {
      requested_read_length = INT32_MAX;
    }
    else
    {
      requested_read_length = n;
    }

    size_t r = 0;
    if (_64_option)
    {
        r = PosixLibcCaller::pread64(fd, dst, requested_read_length, off);
    }
    else
    {
        r = PosixLibcCaller::pread(fd, dst, requested_read_length, off);
    }
    if (r > 0)
    {
      dst += r;
      n -= r;
      bytes_read += r;
      off += r;
    }
    else if (r == 0)
    {
      std::cerr << "Read less bytes than requested." << std::endl;
      error = 1;
    }
    else if (errno == EINTR || errno == EAGAIN)
    {
      // Retry
    }
    else
    {
      std::cerr << "IOError (off:" << off << ", size:" << requested_read_length << "): " << std::strerror(errno) << std::endl;
      error = 1;
    }
  }

  return bytes_read;
}

ssize_t PosixFileSystemDriver::read(int fd, char *result, size_t n, off_t offset, bool _64_option)
{
  ssize_t bytes_read = 0;
  ssize_t total_bytes_read = 0;
  off_t buffer_offset = 0;
  size_t usable_block_size = block_size;
  // Update block size to prevent errors
  if (usable_block_size > n)
  {
    usable_block_size = n;
  }

  // Read file
  while (total_bytes_read < n)
  {
    // Read block
    bytes_read = read_block(fd, result + buffer_offset, usable_block_size, offset, _64_option);
    total_bytes_read += bytes_read;
    offset += bytes_read;
    buffer_offset += bytes_read;
    if (total_bytes_read + usable_block_size > n)
    {
      usable_block_size = n - total_bytes_read;
    }
  }

  return total_bytes_read;
}

Status<ssize_t> PosixFileSystemDriver::read(Info *i, char *result, size_t n, off_t offset)
{
    int fd = open_(get_full_path(i->name).c_str(), O_RDONLY);

    if (fd < 0)
        return {NOT_FOUND};

    ssize_t bytes_read = read(fd, result, n, offset, false);

    close_(fd);
    // Validate result
    return {bytes_read};
}

ssize_t PosixFileSystemDriver::write_block(int fd, char *buf, size_t n, off_t off)
{
  int error = 0;
  ssize_t bytes_written = 0;
  while (n > 0 && error == 0)
  {
    // Some platforms, notably macs, throw EINVAL if pread is asked to read
    // more than fits in a 32-bit integer.
    size_t requested_write_length;
    if (n > INT32_MAX)
    {
      requested_write_length = INT32_MAX;
    }
    else
    {
      requested_write_length = n;
    }
    ssize_t r = pwrite(fd, buf, requested_write_length, off);
    if (r > 0)
    {
      buf += r;
      n -= r;
      bytes_written += r;
      off += r;
    }
    else if (r == 0)
    {
      std::cerr << "Wrote less bytes than requested." << std::endl;
      error = 1;
    }
    else if (errno == EINTR || errno == EAGAIN)
    {
      // Retry
    }
    else
    {
      std::cerr << "Write IOError (off:" << off << ", size:" << requested_write_length << "): " << std::strerror(errno) << std::endl;
      error = 1;
    }
  }
  return bytes_written;
}

ssize_t PosixFileSystemDriver::write(int fd, char *buf, size_t size, off_t offset){
    off_t buffer_offset = 0;
    ssize_t total_bytes_written = 0;
    ssize_t bytes_written;
    size_t usable_block_size = block_size;

    if (usable_block_size > size)
    {
        usable_block_size = size;
    }

    while (total_bytes_written < size)
    {
        // Write block
        bytes_written = write_block(fd, buf + buffer_offset, usable_block_size, offset);
        buffer_offset += bytes_written;
        offset += bytes_written;
        total_bytes_written += bytes_written;

        if (total_bytes_written + usable_block_size > size)
        {
        usable_block_size = size - total_bytes_written;
        }
    }

    return total_bytes_written;
}

// File here has content
Status<ssize_t> PosixFileSystemDriver::write(File *f)
{
    int fd = open_(get_full_path(f->info->name).c_str(), O_RDWR | O_CREAT, 0644);

    if (fd < 0) 
    {
        return {NOT_FOUND};
    }

    ssize_t bytes_written = write(fd, f->content, f->requested_size, f->offset);

    close_(fd);

    return {bytes_written};
}


Status<ssize_t> PosixFileSystemDriver::remove(Info *i)
{
  if (std::remove(get_full_path(i->name).c_str()) == 0)
  {
    return {SUCCESS};
  }
  return {NOT_FOUND};
}


void PosixFileSystemDriver::create_dir(const std::string &path)
{
    char tmp[256];
    char *p = nullptr;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path.c_str());
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++)
        if (*p == '/')
        {
        *p = 0;
        mkdir(tmp, S_IRWXU);
        *p = '/';
        }
    mkdir(tmp, S_IRWXU);
}

StorageDriverSubType PosixFileSystemDriver::get_subtype() {
    return StorageDriverSubType::POSIX;
}

void PosixFileSystemDriver::create_environment(std::vector<std::string> &dirs, bool enable_write)
{
  if (enable_write)
  {
    create_dir(storage_prefix);
    for (auto &dir : dirs)
    {
      create_dir(storage_prefix + dir);
    }
  }
}

int PosixFileSystemDriver::open_(const char *pathname, int flags, mode_t mode){
    return PosixLibcCaller::open(get_full_path(pathname).c_str(), flags, mode);
}

int PosixFileSystemDriver::open_(const char *pathname, int flags){
    return PosixLibcCaller::open(get_full_path(pathname).c_str(), flags);
}

int PosixFileSystemDriver::open64_(const char *pathname, int flags, mode_t mode){
    return PosixLibcCaller::open64(get_full_path(pathname).c_str(), flags, mode);
}

int PosixFileSystemDriver::open64_(const char *pathname, int flags){
    return PosixLibcCaller::open64(get_full_path(pathname).c_str(), flags);
}

ssize_t PosixFileSystemDriver::PosixFileSystemDriver::pread_(int fildes, void *result, size_t nbyte, off_t offset){
    return read(fildes, static_cast<char*>(result), nbyte, offset, false);
}

ssize_t PosixFileSystemDriver::pread64_(int fildes, void *result, size_t nbyte, off_t offset){
    return read(fildes, static_cast<char*>(result), nbyte, offset, true);
}

ssize_t PosixFileSystemDriver::pwrite_(int fildes, char *buf, size_t count, off_t offset){
    return write(fildes, buf, count, offset);
}

void *PosixFileSystemDriver::mmap_(void *addr, size_t length, int prot, int flags, int fd, off_t offset){
    PosixLibcCaller::mmap(addr, length, prot, flags, fd, offset);
}

int PosixFileSystemDriver::close_(int fildes){
    PosixLibcCaller::close(fildes);
}

PosixFileSystemDriverBuilder* PosixFileSystemDriver::create(){
    return new PosixFileSystemDriverBuilder();
}