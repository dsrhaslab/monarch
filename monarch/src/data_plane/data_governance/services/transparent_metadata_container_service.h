//
// Created by dantas on 20/08/22.
//

#ifndef MONARCH_TRANSPARENT_METADATA_CONTAINER_SERVICE_H
#define MONARCH_TRANSPARENT_METADATA_CONTAINER_SERVICE_H

#include "metadata_container_service.h"
#if defined BAZEL_BUILD || defined TF_BAZEL_BUILD
#include "third_party/tbb/include/concurrent_hash_map.h"
#else
#include "concurrent_hash_map.h"
#endif

struct HashCompare {
    static size_t hash(  const int& key )                    { return boost::hash_value(key); }
    static bool   equal( const int& key1, const int& key2 )  { return ( key1 == key2 ); }
};

typedef tbb::concurrent_hash_map<int, Info*, HashCompare> concurrent_map_tbb;


class TransparentMetadataContainerService {
    //TODO possibly not the best concurrent map
    concurrent_map_tbb file_descriptors_to_info_;

public:

    inline Info* get_metadata(int fildes){
        concurrent_map_tbb::const_accessor a;
        if(!file_descriptors_to_info_.find(a, fildes)){
            return nullptr;
        }else{
            Info* i = a->second;
            // Release lock for this filename
            a.release();
            return i;
        }
    };

    inline void store_fildes(int fildes, Info* i){
        concurrent_map_tbb::accessor a;
        bool new_file = file_descriptors_to_info_.insert(a, fildes);

        if (!new_file) {
            // Release lock for this filename
            a.release();
        } else {
            // Add file with no content to buffer
            a->second = i;

            // Release lock for this filename
            a.release();
        }
    };

    inline Info* remove_fildes(int fildes){
        concurrent_map_tbb::const_accessor a;
        bool file_in_buffer = file_descriptors_to_info_.find(a, fildes);
        if(file_in_buffer) {
            Info* i = a->second;
            a.release();
            file_descriptors_to_info_.erase(fildes);
            return i;
        }else{
            return nullptr;
        }
    };
};

#endif //MONARCH_TRANSPARENT_METADATA_CONTAINER_SERVICE_H