//
// Created by dantas on 01/11/20.
//


#ifndef MONARCH_TBB_MEMORY_BUFFER_DRIVER_H
#define MONARCH_TBB_MEMORY_BUFFER_DRIVER_H

#include <boost/functional/hash.hpp>
#include <iostream>
#if defined BAZEL_BUILD || defined TF_BAZEL_BUILD
#include "third_party/tbb/include/concurrent_hash_map.h"
#else
#include "concurrent_hash_map.h"
#endif

#include "memory_buffer_driver.h"

template<typename K>
struct HashCompareG {
    static size_t hash( const K& key )                  { return boost::hash_value(key); }
    static bool   equal( const K& key1, const K& key2 ) { return ( key1 == key2 ); }
};

typedef tbb::concurrent_hash_map<std::string, File*, HashCompareG<std::string>> content_hash_map;

//Thread safe
class TBBMemoryBufferDriver : public MemoryBufferDriver {
    content_hash_map buffer;

public:
    Status<ssize_t> read(Info* i, char* result, size_t n, off_t offset) override;

    Status<ssize_t> write(File* f) override;

    Status<ssize_t> remove(Info* i) override;

    StorageDriverSubType get_subtype() override;

    File* remove_for_copy(Info* i) override;
};

#endif //MONARCH_TBB_MEMORY_BUFFER_DRIVER_H
