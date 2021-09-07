//
// Created by dantas on 19/10/20.
//

#include <fcntl.h>
#include <bits/stdc++.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <iostream>
#include <unistd.h>
#include <cstring>

#include "file_system_driver.h"
#include "absl/strings/str_cat.h"

//TODO warning: comparison of integer expressions of different signedness: 'size_t' {aka 'long unsigned int'} and 'int' [-Wsign-compare]
 //  65 |     if(usable_block_size > file_size)

inline std::basic_string<char> FileSystemDriver::get_full_path(const std::string& filename){
    return absl::StrCat(storage_prefix, "/", filename);
}

ssize_t FileSystemDriver::read_block(int fd, char* result, size_t n, uint64_t off) {
    int error = 0;
    char* dst = result;
    ssize_t bytes_read = 0;

    while (n > 0 && error == 0) {
        // Some platforms, notably macs, throw EINVAL if pread is asked to read
        // more than fits in a 32-bit integer.
        size_t requested_read_length;
        if (n > INT32_MAX) {
            requested_read_length = INT32_MAX;
        } else {
            requested_read_length = n;
        }

        ssize_t r = pread(fd, dst, requested_read_length, static_cast<off_t>(off));

        if (r > 0) {
            dst += r;
            n -= r;
            bytes_read += r;
            off += r;
        } else if (r == 0) {
            std::cerr << "Read less bytes than requested." << std::endl;
            error = 1;
        } else if (errno == EINTR || errno == EAGAIN) {
            // Retry
        } else {
            std::cerr << "IOError (off:" << off << ", size:" << requested_read_length
                      << "): " << std::strerror(errno) << std::endl;
            error = 1;
        }
    }

    return bytes_read;
}


ssize_t FileSystemDriver::read(int fd, char* result, uint64_t offset, size_t n){
    ssize_t bytes_read;
    ssize_t total_bytes_read = 0;
    uint64_t buffer_offset = 0;
    size_t usable_block_size = BaseDataStorageDriver::block_size;
    // Update block size to prevent errors
    if(usable_block_size > n) {
        usable_block_size = n;
    }

    // Read file
    while(total_bytes_read < n) {
        // Read block
        bytes_read = read_block(fd, result + buffer_offset, usable_block_size, offset);
        total_bytes_read += bytes_read;
        offset += bytes_read;
        buffer_offset += bytes_read;
        if(total_bytes_read + usable_block_size > n) {
            usable_block_size = n - total_bytes_read;
        }
    }

    return total_bytes_read;
}

Status FileSystemDriver::read(FileInfo* fi, char* result, uint64_t offset, size_t n){
    int fd;

    if(!fi->has_shareable_file_descriptors()) {
        // Open file
        fd = open(get_full_path(fi->get_name()).c_str(), O_RDONLY);
    }else{
        //open_descriptor needs to be executed by the caller. No need for locking since open has been signaled already
        fd = fi->get_file_descriptor(level);
    }
    if(fd < 0)
        return {NOT_FOUND};

    ssize_t bytes_read = read(fd, result, offset, n);

    if(!fi->has_shareable_file_descriptors()) {
        close(fd);
    }
    // Validate result
    return {bytes_read};
}

void FileSystemDriver::open_descriptor(FileInfo* fi, bool client_open){
    std::unique_lock<std::mutex> ul(*fi->get_mutex());
    auto& di = fi->get_descriptor_info(BaseDataStorageDriver::level);
    if(std::get<0>(di) == -1){
        //update fd
        std::get<0>(di) = open(get_full_path(fi->get_name()).c_str(), O_RDONLY);
    }
    if(client_open){
        //if its first client read signal open
        if(!std::get<2>(di)){
            //set client reading to true
            std::get<2>(di) = true;
            //signal open;
            std::get<1>(di)++;
        }
    }else{
        //signal open;
        std::get<1>(di)++;
    }
}

void FileSystemDriver::close_descriptor(FileInfo* fi){
    std::unique_lock<std::mutex> ul(*fi->get_mutex());
    auto& di = fi->get_descriptor_info(BaseDataStorageDriver::level);
    //signal close;
    if(--std::get<1>(di) == 0){
        close(std::get<0>(di));
        std::get<0>(di) = -1;
    }
}

Status FileSystemDriver::read(File* f){
    return read(f->get_info(), f->get_content(), f->get_offset(), f->get_requested_size());
}


ssize_t FileSystemDriver::write_block(int fd, char* buf, size_t n, uint64_t off) {
    int error = 0;
    ssize_t bytes_written = 0;
    while (n > 0 && error == 0) {
        // Some platforms, notably macs, throw EINVAL if pread is asked to read
        // more than fits in a 32-bit integer.
        size_t requested_write_length;
        if (n > INT32_MAX) {
            requested_write_length = INT32_MAX;
        } else {
            requested_write_length = n;
        }

        ssize_t r = pwrite(fd, buf, requested_write_length, static_cast<off_t>(off));
        if (r > 0) {
            buf += r;
            n -= r;
            bytes_written += r;
            off += r;
        } else if (r == 0) {
            std::cerr << "Wrote less bytes than requested." << std::endl;
            error = 1;
        } else if (errno == EINTR || errno == EAGAIN) {
            // Retry
        } else {
            std::cerr << "Write IOError (off:" << off << ", size:" << requested_write_length
                      << "): " << std::strerror(errno) << std::endl;
            error = 1;
        }
    }
    return bytes_written;
}


// File here has content
Status FileSystemDriver::write(File* f) {
    std::string file_path = get_full_path(f->get_filename());
    //std::cout << "writting " << file_path << "\n";
    int fd = open(file_path.c_str(), O_RDWR | O_CREAT, 0644);

    if(fd < 0)
        return {NOT_FOUND};

    uint64_t offset = f->get_offset();
    uint64_t buffer_offset = 0;
    ssize_t total_bytes_written = 0;
    ssize_t bytes_written;
    size_t size = f->get_requested_size();
    size_t usable_block_size = BaseDataStorageDriver::block_size;

    if(usable_block_size > size) {
        usable_block_size = size;
    }

    while(total_bytes_written < size ) {
        // Write block
        bytes_written = write_block(fd, f->get_content() + buffer_offset, usable_block_size, static_cast<off_t>(offset));
        buffer_offset += bytes_written;
        offset += bytes_written;
        total_bytes_written += bytes_written;

        if(total_bytes_written + usable_block_size > size) {
            usable_block_size = size - total_bytes_written;
        }
    }
    close(fd);
    return {total_bytes_written};
}

Status FileSystemDriver::remove(FileInfo* fi){
   if (std::remove(get_full_path(fi->get_name()).c_str()) == 0) {
       return {SUCCESS};
   }
   return {NOT_FOUND};
}

ssize_t FileSystemDriver::sizeof_content(FileInfo* fi){
    return fi->_get_size();
}

bool FileSystemDriver::in_memory_type(){
    return false;
}

bool FileSystemDriver::file_system_type(){
    return true;
}

File* FileSystemDriver::remove_for_copy(FileInfo* fi){
    std::cerr << "remove_for_copy is not available in disk type driver" << std::endl;
    exit(1);
}

void FileSystemDriver::create_dir(const std::string& path){
    std::string cmd = "mkdir -p " + path;
    int status = std::system(cmd.c_str());
    if (status == -1)
        std::cerr << "Error : " << strerror(errno) << std::endl;
    else
        std::cout << "Created " << path << std::endl;
}

void FileSystemDriver::create_environment(std::vector<std::string>& dirs){
    //needed since dirs can be empty;
    create_dir(storage_prefix);
    for(auto& dir : dirs){
        create_dir(storage_prefix + dir);
    }
}
