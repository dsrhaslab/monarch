//
// Created by dantas on 26/03/21.
//

#ifndef THESIS_METADATA_CONTAINER_SERVICE_H
#define THESIS_METADATA_CONTAINER_SERVICE_H

#include <string>
#include <vector>
#include <iostream>
#include <boost/functional/hash.hpp>

#include "absl/container/flat_hash_map.h"
#include "file_info.h"
#if defined BAZEL_BUILD || defined TF_BAZEL_BUILD
#include "third_party/tbb/include/concurrent_hash_map.h"
#else
#include "concurrent_hash_map.h"
#endif
//TODO make it templated

struct HashCompare {
    static size_t hash( const int& key )                  { return boost::hash_value(key); }
    static bool   equal( const int& key1, const int& key2 ) { return ( key1 == key2 ); }
};

typedef tbb::concurrent_hash_map<int, FileInfo*, HashCompare> concurrent_map_tbb;

template<class T>
class MetadataContainerService {
    static_assert(std::is_base_of<FileInfo, T>::value, "MetadataContainerService must be derived from FileInfo");
    int distributed_id;
    absl::flat_hash_map<std::string, int> target_class_to_id;
    absl::flat_hash_map<std::string, T*> name_to_info;
    absl::flat_hash_map<int, T*> id_to_info;
    //TODO possibly not the best concurrent map
    concurrent_map_tbb file_descriptors_to_info;
    std::vector<std::tuple<std::string, int>> dirs_file_count;
    int world_size;
    int file_count;
    int partition_file_count;
    size_t full_size;
    int epochs;
    std::vector<std::vector<int>> partitioned_samples_ordered_ids;
    int current_epoch;
    int storage_source_level;

    bool has_shareable_file_descriptors;
    bool stores_ids;

    bool local_parse;
    std::string data_dir;
    std::string type;

    std::string train_files_regex;
    size_t train_full_size;

public:
    MetadataContainerService();
    MetadataContainerService(const std::string& data_dir, const std::string& type);
    void set_distributed_id(int id);
    void set_world_size(int world_size_);
    void set_file_count(int file_count);
    void set_full_size(size_t full_size);
    void set_epochs(int epochs);
    void set_storage_source_level(int source_level);
    void set_shareable_file_descriptors(bool value);
    void set_stores_ids(bool value);
    void set_stores_targets(bool value);
    void set_train_files_regex(std::string& regex);
    void add_dir_file_count(const std::string& name, size_t count);
    void add_target_class_to_id(const std::string& name, int id);
    void add_entry(int id, T* fi);

    void make_partition(int rank_, int world_size_, int worker_id_, int num_workers_);
    //called once
    void generate_samples_ordered_ids();
    //called for each epoch
    void generate_samples_ordered_ids(const std::vector<int>& shuffling_seeds);

    T* get_metadata(const std::string& name);
    T* get_metadata(int id);
    T* get_metadata_from_fildes(int fildes);
    T* get_metadata_from_ordered_id(int rank, int id);
    void store_fildes(int fildes, T*);
    T* remove_fildes(int fildes);
    int get_id(int rank, int index);
    T* get_file(int index);
    int get_current_epoch();
    int get_iter_size();
    size_t get_full_size();
    size_t get_train_full_size();
    bool is_train_file(const std::string& filename);
    int get_file_count();
    std::vector<std::string> get_dirs_for_environment();
    FileInfo* parse_file(const std::string& path, const std::string& filename, const std::string& type);
    void execute_local_parse();
    void init();
};


#endif //THESIS_METADATA_CONTAINER_SERVICE_H
