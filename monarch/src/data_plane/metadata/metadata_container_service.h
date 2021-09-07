//
// Created by dantas on 26/03/21.
//

#ifndef THESIS_METADATA_CONTAINER_SERVICE_H
#define THESIS_METADATA_CONTAINER_SERVICE_H

#include <string>
#include <vector>
#include "absl/container/flat_hash_map.h"

#include "file_info.h"

//TODO make it templated

template<class T>
class MetadataContainerService {
    static_assert(std::is_base_of<FileInfo, T>::value, "I must be derived from FileInfo");
    int distributed_id;
    absl::flat_hash_map<std::string, int> target_class_to_id;
    absl::flat_hash_map<std::string, T*> name_to_info;
    absl::flat_hash_map<int, T*> id_to_info;
    std::vector<std::tuple<std::string, int>> dirs_file_count;
    int world_size;
    int file_count;
    int partition_file_count;
    size_t full_size;
    int epochs;
    std::vector<std::vector<int>> partitioned_samples_ordered_ids;
    int storage_source_level;
    int current_epoch;
    int file_index;
    bool erroneous_state;

public:
    MetadataContainerService();
    void set_distributed_id(int id);
    void set_world_size(int world_size_);
    void set_file_count(int file_count);
    void set_full_size(size_t full_size);
    void set_epochs(int epochs);
    void set_storage_source_level(int level);
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
    T* get_metadata_from_ordered_id(int rank, int id);
    int get_id(int rank, int index);
    T* get_file(int index);
    int get_current_epoch();
    int get_iter_size();
    size_t get_full_size();
    int get_file_count();
    std::vector<std::string> get_dirs_for_environment();
};


#endif //THESIS_METADATA_CONTAINER_SERVICE_H
