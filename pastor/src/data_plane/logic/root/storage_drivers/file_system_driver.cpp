//
// Created by dantas on 19/10/20.
//

#include <fcntl.h>
#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <dlfcn.h>

#include "file_system_driver.h"
#include "absl/strings/str_cat.h"
#include "libraries/libc_operations_enums.h"
#include "libraries/libc_operation_headers.h"

inline std::basic_string<char> FileSystemDriver::get_full_path(const std::string& filename){
    return absl::StrCat(storage_prefix, filename);
}

int FileSystemDriver::passthrough_lib_open(const char *pathname, int flags, mode_t mode, bool _64_option){
    if(_64_option){
        if(m_lib_handle && !m_metadata_operations.m_open64_var) {
            m_metadata_operations.m_open64_var = (libc_open64_variadic_t) dlsym(m_lib_handle, "open64");
        }else if(!m_metadata_operations.m_open64_var){
            m_metadata_operations.m_open64_var = (libc_open64_variadic_t) dlsym(RTLD_NEXT, "open64");
        }
        return m_metadata_operations.m_open64_var(pathname, flags, mode);
    }else{
        if(m_lib_handle && !m_metadata_operations.m_open_var) {
            m_metadata_operations.m_open_var = (libc_open_variadic_t) dlsym(m_lib_handle, "open");
        }else if(!m_metadata_operations.m_open_var){
            m_metadata_operations.m_open_var = (libc_open_variadic_t) dlsym(RTLD_NEXT, "open");
        }
        return m_metadata_operations.m_open_var(pathname, flags, mode);
    }
}

int FileSystemDriver::passthrough_lib_open(const char *pathname, int flags, bool _64_option){
    if(_64_option){
        if(m_lib_handle && !m_metadata_operations.m_open64) {
            m_metadata_operations.m_open64 = (libc_open64_t) dlsym(m_lib_handle, "open64");
        }else if(!m_metadata_operations.m_open64){
            m_metadata_operations.m_open64 = (libc_open64_t) dlsym(RTLD_NEXT, "open64");
        }
        return m_metadata_operations.m_open64(pathname, flags);
    }else{
        if(m_lib_handle && !m_metadata_operations.m_open) {
            m_metadata_operations.m_open = (libc_open_t) dlsym(m_lib_handle, "open");
        }else if(!m_metadata_operations.m_open){
            m_metadata_operations.m_open = (libc_open_t) dlsym(RTLD_NEXT, "open");
        }
        return m_metadata_operations.m_open(pathname, flags);
    }
}

ssize_t FileSystemDriver::passthrough_lib_pread(int fildes, char *result, size_t nbyte, uint64_t offset, bool _64_option){
    if(_64_option){
        if(!m_lib_handle && !m_data_operations.m_pread64){
            m_data_operations.m_pread64 = (libc_pread64_t) dlsym(RTLD_NEXT, "pread64");
        }else if(!m_data_operations.m_pread64){
            m_data_operations.m_pread64 = (libc_pread64_t) dlsym(m_lib_handle, "pread64");
        }
        return m_data_operations.m_pread64(fildes, result, nbyte, static_cast<off_t>(offset));
    }else {
        if (!m_lib_handle && !m_data_operations.m_pread) {
            m_data_operations.m_pread = (libc_pread_t) dlsym(RTLD_NEXT, "pread");
        }else if(!m_data_operations.m_pread) {
            m_data_operations.m_pread = (libc_pread_t) dlsym(m_lib_handle, "pread");
        }
        return m_data_operations.m_pread(fildes, result, nbyte, static_cast<off_t>(offset));
    }
}

int FileSystemDriver::passthrough_lib_close(int fildes){
    if(!m_lib_handle && !m_metadata_operations.m_close){
        m_metadata_operations.m_close = (libc_close_t) dlsym(RTLD_NEXT, "close");
    }else if(!m_metadata_operations.m_close){
        m_metadata_operations.m_close = (libc_close_t) dlsym(m_lib_handle, "close");
    }
    return m_metadata_operations.m_close(fildes);
}

void *FileSystemDriver::passthrough_lib_mmap (void *addr, size_t length, int prot, int flags, int fd, off_t offset){
    if(!m_lib_handle && !m_data_operations.m_mmap){
        m_data_operations.m_mmap = (libc_mmap_t) dlsym(RTLD_NEXT, "mmap");
    }else if (!m_data_operations.m_mmap){
        m_data_operations.m_mmap = (libc_mmap_t) dlsym(m_lib_handle, "mmap");
    }
    return m_data_operations.m_mmap(addr, length, prot, flags, fd, offset);
}

ssize_t FileSystemDriver::read_block(int fd, char* result, size_t n, uint64_t off, bool _64_option) {
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

        size_t r = 0;
        if(transparent_api){
            r = passthrough_lib_pread(fd, dst, requested_read_length, off, _64_option);
        }else {
            if(_64_option){
                r = pread64(fd, dst, requested_read_length, static_cast<off_t>(off));
            }else {
                r = pread(fd, dst, requested_read_length, static_cast<off_t>(off));
            }
        }

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


ssize_t FileSystemDriver::read(int fd, char* result, uint64_t offset, size_t n, bool _64_option){
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
        bytes_read = read_block(fd, result + buffer_offset, usable_block_size, offset, _64_option);
        total_bytes_read += bytes_read;
        offset += bytes_read;
        buffer_offset += bytes_read;
        if(total_bytes_read + usable_block_size > n) {
            usable_block_size = n - total_bytes_read;
        }
    }

    return total_bytes_read;
}

Status<ssize_t> FileSystemDriver::read(FileInfo* fi, char* result, uint64_t offset, size_t n, bool _64_option){
    int fd;

    if(!fi->has_shareable_file_descriptors()) {
        // Open file
        if(transparent_api){
            fd = passthrough_lib_open(get_full_path(fi->get_name()).c_str(), O_RDONLY, false);
        }else{
            fd = open(get_full_path(fi->get_name()).c_str(), O_RDONLY);
        }
    }else{
        //open_descriptor needs to be executed by the caller. No need for locking since open has been signaled already
        fd = fi->get_file_descriptor(level);
    }
    if(fd < 0)
        return {NOT_FOUND};

    ssize_t bytes_read = read(fd, result, offset, n, _64_option);

    if(!fi->has_shareable_file_descriptors()) {
        if(transparent_api){
            passthrough_lib_close(fd);
        }else {
            close(fd);
        }
    }
    // Validate result
    return {bytes_read};
}


int FileSystemDriver::open_descriptor(FileInfo* fi, int flags, mode_t mode, bool _64_option, bool client_open){
    std::unique_lock<std::mutex> ul(*fi->get_mutex());
    auto& di = fi->get_descriptor_info(BaseDataStorageDriver::level);
    //update fd
    int n_readers = client_open ? std::get<1>(di)++ + std::get<2>(di) : std::get<1>(di) + std::get<2>(di)++;
    if(n_readers == 0) {
        if (transparent_api) {
            std::get<0>(di) = passthrough_lib_open(get_full_path(fi->get_name()).c_str(), flags, mode, _64_option);
        } else {
            std::get<0>(di) = open(get_full_path(fi->get_name()).c_str(), flags, mode);
        }
        //int c = std::get<1>(di);
        //int p = std::get<2>(di);
        //int fd = std::get<0>(di);
        //std::cout << "--" << fi->get_name() << "--" << "->successful_open on level: " << BaseDataStorageDriver::level << " c_value: "  << c << ", p_value: " << p << " ,result: " << fd << std::endl;
        return 1;
    }
    //int c = std::get<1>(di);
    //int p = std::get<2>(di);
    //std::cout << "--" << fi->get_name() << "--" << "->open on level: " << BaseDataStorageDriver::level << " c_value: "  << c << ", p_value: " << p << std::endl;
    return 0;
}

int FileSystemDriver::open_descriptor(FileInfo* fi, int flags, bool _64_option, bool client_open){
    std::unique_lock<std::mutex> ul(*fi->get_mutex());
    auto& di = fi->get_descriptor_info(BaseDataStorageDriver::level);
    //update fd
    int n_readers = client_open ? std::get<1>(di)++ + std::get<2>(di) : std::get<1>(di) + std::get<2>(di)++;
    if(n_readers == 0) {
        if (transparent_api) {
            std::get<0>(di) = passthrough_lib_open(get_full_path(fi->get_name()).c_str(), flags, _64_option);
        } else {
            std::get<0>(di) = open(get_full_path(fi->get_name()).c_str(), flags);
        }
        //int c = std::get<1>(di);
        //int p = std::get<2>(di); 
        //int fd = std::get<0>(di);
        //std::cout << "--" << fi->get_name() << "--" << "->successful_open on level: " << BaseDataStorageDriver::level << " c_value: "  << c << ", p_value: " << p << " ,result: " << fd << std::endl;
        return 1;
    }
    //int c = std::get<1>(di);
    //int p = std::get<2>(di);     
    //std::cout << "--" << fi->get_name() << "--" << "->open on level: " << BaseDataStorageDriver::level << " c_value: "  << c << ", p_value: " << p << std::endl;
    return 0;
}

int FileSystemDriver::close_descriptor(FileInfo* fi, bool client_close){
    std::unique_lock<std::mutex> ul(*fi->get_mutex());
    auto& di = fi->get_descriptor_info(BaseDataStorageDriver::level);
    //ignore close to not open fd. This occurs when placement is slower than client read.
    if(std::get<0>(di) == -1){
        return 0;
    }
    //Ignore close when client has't read. This avoid the case where placement opens and the client closes right after it. This would cause the placement write to fail
    if(client_close && std::get<1>(di) == 0){
        return 0;
    }
 
    int n_readers = client_close ? --std::get<1>(di) + std::get<2>(di) : std::get<1>(di) + --std::get<2>(di);
    if(n_readers == 0){
        //int res = 0;
        if(transparent_api){
            passthrough_lib_close(std::get<0>(di));
        }else{
            close(std::get<0>(di));
        }
        //int c = std::get<1>(di);
        //int p = std::get<2>(di);
        //int last_fd = std::get<0>(di);
        //std::cout << "--" <<  fi->get_name() << "--" << "->final_close on level: " << BaseDataStorageDriver::level << " c_value: " << c << ", p_value: " << p << " ,result: " << res << " ,last_fd: " << last_fd << std::endl;
        std::get<0>(di) = -1;
        return 1;
    }
    //int c = std::get<1>(di);
    //int p = std::get<2>(di);
    //int fd = std::get<0>(di);
    //std::cout << "--" << fi->get_name() << "--" << "->close on level: " << BaseDataStorageDriver::level << " c_value: " << c << ", p_value: " << p << " ,fd: " << fd << std::endl;
    return 0;
}

//Used by the placement handler. Guarantees that the file descriptor for this level is only closed when no one is reading from the upper level and this level.
//Used to counter the race condition of having the same file being read on different epochs. 
int FileSystemDriver::upper_level_conditional_close_descriptor(FileInfo* fi){
    std::unique_lock<std::mutex> ul(*fi->get_mutex());
    
    auto& di_floor = fi->get_descriptor_info(BaseDataStorageDriver::level);

    if(std::get<0>(di_floor) == -1){
        return 0;
    }

    auto& di_upper = fi->get_descriptor_info(BaseDataStorageDriver::level + 1);   
 
    int n_readers = std::get<1>(di_floor) + --std::get<2>(di_floor) + std::get<1>(di_upper) + std::get<2>(di_upper);
    if(n_readers == 0){
        //int res = 0;
        if(transparent_api){
            passthrough_lib_close(std::get<0>(di_floor));
        }else{
            close(std::get<0>(di_floor));
        }
        //int c = std::get<1>(di_floor);
        //int p = std::get<2>(di_floor);
        //int c_upper = std::get<1>(di_upper);
        //int p_upper = std::get<2>(di_upper);
        //int last_fd = std::get<0>(di_floor);
        //std::cout << "--" <<  fi->get_name() << "--" << "->final_conditional_close on level: " << BaseDataStorageDriver::level << " c_floor: " << c << ", p_floor: " << p 
        //          << "; c_upper: " << c_upper << ", p_upper: " << p_upper << " ,result: " << res << " ,last_fd: " << last_fd << std::endl;
        std::get<0>(di_floor) = -1;
        return 1;
    }
    //int c = std::get<1>(di_floor);
    //int p = std::get<2>(di_floor);
    //int c_upper = std::get<1>(di_upper);
    //int p_upper = std::get<2>(di_upper);
    //int fd = std::get<0>(di_floor);
    //std::cout << "--" <<  fi->get_name() << "--" << "->final_conditional_close on level: " << BaseDataStorageDriver::level << " c_floor: " << c << ", p_floor: " << p
    //          << "; c_upper: " << c_upper << ", p_upper: " << p_upper << " ,fd: " << fd << std::endl;
    return 0;
}

Status<ssize_t> FileSystemDriver::read(File* f){
    return read(f->get_info(), f->get_content(), f->get_offset(), f->get_requested_size(), false);
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
Status<ssize_t> FileSystemDriver::write(File* f) {
    int fd;
    auto* fi = f->get_info();
    if(!fi->has_shareable_file_descriptors()) {
        // Open file
        if(transparent_api){
            fd = passthrough_lib_open(get_full_path(fi->get_name()).c_str(), O_RDWR | O_CREAT, 0644, false);
        }else{
            fd = open(get_full_path(fi->get_name()).c_str(), O_RDWR | O_CREAT, 0644);
        }
    }else{
        //open_descriptor needs to be executed by the caller. No need for locking since open has been signaled already
        fd = fi->get_file_descriptor(level);
    }

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

    if(!fi->has_shareable_file_descriptors()) {
        if(transparent_api){
            passthrough_lib_close(fd);
        }else {
            close(fd);
        }
    }
    return {total_bytes_written};
}

Status<ssize_t> FileSystemDriver::remove(FileInfo* fi){
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
    if(!transparent_api) {
        std::string cmd = "mkdir -p " + path;
        int status = std::system(cmd.c_str());
        if (status == -1)
            std::cerr << "Error : " << strerror(errno) << std::endl;
        else
            std::cout << "Created " << path << std::endl;
    }else{
        char tmp[256];
        char *p = nullptr;
        size_t len;

        snprintf(tmp, sizeof(tmp),"%s", path.c_str());
        len = strlen(tmp);
        if(tmp[len - 1] == '/')
            tmp[len - 1] = 0;
        for(p = tmp + 1; *p; p++)
            if(*p == '/') {
                *p = 0;
                mkdir(tmp, S_IRWXU);
                *p = '/';
            }
        mkdir(tmp, S_IRWXU);
    }
}

void FileSystemDriver::create_environment(std::vector<std::string>& dirs, bool enable_write){
    if(enable_write) {
        create_dir(storage_prefix);
        for (auto &dir : dirs) {
            create_dir(storage_prefix + dir);
        }
    }
}

void FileSystemDriver::dlopen_library_handle(){
    m_lib_handle = ::dlopen(m_lib_name.c_str(), RTLD_LAZY);
    if(m_lib_handle == nullptr){
        std::cerr << "Error while dlopen'ing " << m_lib_name << "." << std::endl;
    }
}

void FileSystemDriver::enable_transparent_api(){
    transparent_api = true;
    dlopen_library_handle();
}
