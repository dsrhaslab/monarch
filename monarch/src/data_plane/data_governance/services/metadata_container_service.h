//
// Created by dantas on 26/03/21.
//

#ifndef MONARCH_METADATA_CONTAINER_SERVICE_H
#define MONARCH_METADATA_CONTAINER_SERVICE_H

#include <string>
#include <vector>
#include <iostream>
#include <boost/functional/hash.hpp>

#include "absl/container/flat_hash_map.h"
#if defined BAZEL_BUILD || defined TF_BAZEL_BUILD
#include "third_party/tbb/include/concurrent_hash_map.h"
#else
#include "concurrent_hash_map.h"
#include "../metadata/info.h"
#include "../../types.h"

#endif

class MetadataContainerServiceBuilder;

class MetadataContainerService {
    absl::flat_hash_map<std::string, int> target_class_to_id;
    absl::flat_hash_map<std::string, Info*> name_to_info;
    std::vector<std::tuple<std::string, int>> dirs_file_count;

    int file_count;
    size_t full_size;
    std::vector<std::vector<int>> partitioned_samples_ordered_ids;
    int storage_source_level;

    bool local_parse;
    std::string data_dir_;

    std::string train_files_prefix;
    size_t train_full_size;

protected:
    MetadataContainerService(const std::string& data_dir);

    friend MetadataContainerServiceBuilder;
public:
    //This is updated during the user configuration and by the internal code, since many states and options
    //are consequences of other internal options, hence it does not make sense to ask the user to specify them.
    MetadataType metadata_type_;

    static MetadataContainerServiceBuilder create(const std::string& data_dir, int source_level);
    void set_file_count(int file_count);
    void set_full_size(size_t full_size);
    void add_dir_file_count(const std::string& name, size_t count);
    void add_entry(Info* fi);

    Info* get_metadata(const std::string& name);

    size_t get_full_size();
    size_t get_train_full_size();
    bool is_train_file(const std::string& filename);
    int get_file_count();
    std::vector<std::string> get_dirs_for_environment();
    Info* parse_file(const std::string& path, const std::string& filename);
    void execute_local_parse();
    void init();
};


#endif //MONARCH_METADATA_CONTAINER_SERVICE_H
