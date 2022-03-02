//
// Created by dantas on 01/11/20.
//


#ifndef THESIS_TBB_MEMORY_BUFFER_DRIVER_H
#define THESIS_TBB_MEMORY_BUFFER_DRIVER_H

#include <boost/functional/hash.hpp>
#include "data_storage_driver.h"
#include <iostream>
#if defined BAZEL_BUILD || defined TF_BAZEL_BUILD
#include "third_party/tbb/include/concurrent_hash_map.h"
#else
#include "concurrent_hash_map.h"
#endif

template<typename K>
struct HashCompareG {
    static size_t hash( const K& key )                  { return boost::hash_value(key); }
    static bool   equal( const K& key1, const K& key2 ) { return ( key1 == key2 ); }
};

typedef tbb::concurrent_hash_map<std::string, File*, HashCompareG<std::string>> content_hash_map;

//Thread safe
class TBBMemoryBufferDriver : public BaseDataStorageDriver {
    content_hash_map buffer;

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

#endif //THESIS_TBB_MEMORY_BUFFER_DRIVER_H
