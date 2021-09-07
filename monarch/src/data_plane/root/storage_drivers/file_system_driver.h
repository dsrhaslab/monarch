//
// Created by dantas on 19/10/20.
//

#ifndef THESIS_FILE_SYSTEM_DRIVER_H
#define THESIS_FILE_SYSTEM_DRIVER_H

#include <atomic>
#include "../../metadata/file.h"
#include "data_storage_driver.h"

class FileSystemDriver: public BaseDataStorageDriver{
private:
    ssize_t read(int fd, char* result, uint64_t offset, size_t n);
    ssize_t read_block(int fd, char* result, size_t n, uint64_t off);
    ssize_t write_block(int fd, char* buf, size_t n, uint64_t off);
    std::basic_string<char> get_full_path(const std::string& filename);
    static void create_dir(const std::string& path);
public:
    Status read(FileInfo* fi, char* result, uint64_t offset, size_t n) override;
    Status read(File* f) override;
    Status write(File* f) override;
    Status remove(FileInfo* fi) override;
    ssize_t sizeof_content(FileInfo* fi) override;
    bool in_memory_type() override;
    bool file_system_type() override;
    void create_environment(std::vector<std::string>& dirs) override;
    File* remove_for_copy(FileInfo* fi) override;
    void close_descriptor(FileInfo* fi);
    void open_descriptor(FileInfo* fi, bool client_open);
};

#endif //THESIS_FILE_SYSTEM_DRIVER_H
