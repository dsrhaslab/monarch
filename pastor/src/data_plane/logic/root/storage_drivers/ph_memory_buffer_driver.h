//
// Created by dantas on 24/05/21.
//

#ifndef THESIS_PH_MEMORY_BUFFER_DRIVER_H
#define THESIS_PH_MEMORY_BUFFER_DRIVER_H

#include "data_storage_driver.h"
#include <string>
#include <iostream>
#if defined BAZEL_BUILD || defined TF_BAZEL_BUILD
#include "third_party/parallel_hashmap/phmap.h"
#else
#include "phmap.h"
#endif

class PHMemoryBufferDriver : public BaseDataStorageDriver {
    phmap::parallel_node_hash_map<std::string, File*,
                                  std::hash<std::string>,
                                  std::equal_to<>,
                                  std::allocator<std::pair<const std::string, File*>>,
                                  4,
                                  std::mutex> buffer;
public:
    Status<ssize_t> read(FileInfo* fi, char* result, uint64_t offset, size_t n, bool _64_option) override;
    Status<ssize_t> read(File* f) override;
    Status<ssize_t> write(File* f) override;
    Status<ssize_t> remove(FileInfo* fi) override;
    ssize_t sizeof_content(FileInfo* fi) override;
    bool in_memory_type() override;
    bool file_system_type() override;
    File* remove_for_copy(FileInfo* fi) override;
    void create_environment(std::vector<std::string>& dirs, bool enable_write) override;
};


#endif //THESIS_PH_MEMORY_BUFFER_DRIVER_H
