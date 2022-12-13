//
// Created by dantas on 24/05/21.
//

#ifndef MONARCH_PH_MEMORY_BUFFER_DRIVER_H
#define MONARCH_PH_MEMORY_BUFFER_DRIVER_H

#include <string>
#include <iostream>

#if defined BAZEL_BUILD || defined TF_BAZEL_BUILD
#include "third_party/parallel_hashmap/phmap.h"
#else
#include "phmap.h"
#endif

#include "memory_buffer_driver.h"

class PHMemoryBufferDriver : public MemoryBufferDriver {
    phmap::parallel_node_hash_map<std::string, File*,
                                  std::hash<std::string>,
                                  std::equal_to<>,
                                  std::allocator<std::pair<const std::string, File*>>,
                                  4,
                                  std::mutex> buffer;
public:
    Status<ssize_t> read(Info* i, char* result, size_t n, off_t offset) override;

    Status<ssize_t> write(File* f) override;

    Status<ssize_t> remove(Info* i) override;

    StorageDriverSubType get_subtype() override;

    File* remove_for_copy(Info* i) override;
};


#endif //MONARCH_PH_MEMORY_BUFFER_DRIVER_H
