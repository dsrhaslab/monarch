//
// Created by dantas on 19/10/20.
//

#ifndef THESIS_FILE_SYSTEM_DRIVER_H
#define THESIS_FILE_SYSTEM_DRIVER_H

#include <atomic>
#include "../../metadata/file.h"
#include "data_storage_driver.h"
#include "libraries/libc_operations_enums.h"
#include "libraries/libc_operation_headers.h"

class FileSystemDriver: public BaseDataStorageDriver{
private:
    bool transparent_api;
    std::string m_lib_name { "libc.so.6" };
    void* m_lib_handle { nullptr };
    libc_metadata m_metadata_operations {};
    libc_data m_data_operations {};


private:
    ssize_t read(int fd, char* result, uint64_t offset, size_t n, bool _64_option);
    ssize_t read_block(int fd, char* result, size_t n, uint64_t off, bool _64_option);
    ssize_t write_block(int fd, char* buf, size_t n, uint64_t off);
    std::basic_string<char> get_full_path(const std::string& filename);
    void create_dir(const std::string& path);
    void update_descriptor(std::tuple<int, int, bool>& di, bool client_open);
    void dlopen_library_handle();

public:
    Status<ssize_t> read(FileInfo* fi, char* result, uint64_t offset, size_t n, bool _64_option) override;
    Status<ssize_t> read(File* f) override;
    Status<ssize_t> write(File* f) override;
    Status<ssize_t> remove(FileInfo* fi) override;
    ssize_t sizeof_content(FileInfo* fi) override;
    bool in_memory_type() override;
    bool file_system_type() override;
    void create_environment(std::vector<std::string>& dirs, bool enable_write) override;
    File* remove_for_copy(FileInfo* fi) override;
    int close_descriptor(FileInfo* fi, bool client_close);
    int upper_level_conditional_close_descriptor(FileInfo* fi);
    int open_descriptor(FileInfo* fi, int flags, mode_t mode, bool _64_option, bool client_open);
    int open_descriptor(FileInfo* fi, int flags, bool _64_option, bool client_open);
    void enable_transparent_api();
    int passthrough_lib_close(int fildes);
    int passthrough_lib_open(const char *pathname, int flags, mode_t mode, bool _64_option);
    int passthrough_lib_open(const char *pathname, int flags, bool _64_option);
    ssize_t passthrough_lib_pread(int fildes, char *result, size_t nbyte, uint64_t offset, bool _64_option);
    void *passthrough_lib_mmap (void *addr, size_t length, int prot, int flags, int fd, off_t offset);
};

#endif //THESIS_FILE_SYSTEM_DRIVER_H
