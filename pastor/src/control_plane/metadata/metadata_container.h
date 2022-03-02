//
// Created by dantas on 14/02/21.
//

#ifndef THESIS_METADATA_CONTAINER_H
#define THESIS_METADATA_CONTAINER_H

#include <unordered_map>
#include <map>
#include <vector>
#include <tuple>
#include <memory>

#ifdef BAZEL_BUILD
#include "protos/controller_service.grpc.pb.h"
#elif TF_BAZEL_BUILD
#include "tensorflow/core/platform/pastor/protos/controller_service.grpc.pb.h"
#else
#include "controller_service.grpc.pb.h"
#endif

using controllerservice::ControllerFileInfo;
using controllerservice::ControllerMetadataContainer;

class MetadataContainer {
    std::string regex;
    int target_count;
    bool has_subdirs;
    std::vector<std::tuple<std::string, long>> dirs_file_count;

    int create_target_entry(const std::string& target_dir);
    ControllerFileInfo* parse_file(const std::string& path, const std::string& filename);
public:
    std::string data_dir;
    std::unordered_map<std::string, int> target_class_to_id;
    std::vector<ControllerFileInfo*> infos;
    int file_count;
    size_t full_size;
    int epochs;
    int storage_source_level;
    bool shuffling_enabled;
    std::vector<int> shuffling_seeds;

    MetadataContainer(const std::string& data_dir, const std::string& regex, int hierarchy_size, int epochs);
    void parse();
    std::vector<std::tuple<std::string, long>> get_data_source_infos();
    std::vector<std::string> configs();
};


#endif //THESIS_METADATA_CONTAINER_H
