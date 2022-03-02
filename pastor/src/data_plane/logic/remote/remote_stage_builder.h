//
// Created by dantas on 28/02/21.
//

#if !defined(THESIS_REMOTE_STAGE_BUILDER_H) && defined(INCLUDE_GRPC)
#define THESIS_REMOTE_STAGE_BUILDER_H

#include <cstdio>
#include <thread>
#include <atomic>
#include <grpcpp/grpcpp.h>

#include "../metadata/metadata_container_service.h"

#ifdef BAZEL_BUILD
#include "protos/controller_service.grpc.pb.h"
#elif TF_BAZEL_BUILD
#include "tensorflow/core/platform/pastor/protos/controller_service.grpc.pb.h"
#else
#include "controller_service.grpc.pb.h"
#endif

using grpc::Channel;

using controllerservice::Controller;
using controllerservice::Configuration;

class RemoteStageBuilder {
    std::unique_ptr<Controller::Stub> stub_;
    std::string group;
    std::string dp_server_addr;

    //informations retrieved from controller
    int instance_identifier;
    int world_size;
    int number_of_workers;
    int storage_source_level;
    MetadataContainerService<FileInfo>* metadata_container;
    Configuration reply;

    std::shared_ptr<Channel> get_channel(const std::string &c_server_addr);
    void build_partial_metadata_container(Configuration& reply);

public:
    RemoteStageBuilder(const std::string &group, const std::string &c_server_addr, const std::string &dp_server_addr);
    //return configuration
    std::string get_configuration();
    MetadataContainerService<FileInfo>* get_metadata_container(const std::string& type, bool has_shareable_file_descriptors);
    int get_instance_identifier();
    int get_world_size();
    int get_number_of_workers();
    void synchronize();
};


#endif //THESIS_REMOTE_STAGE_BUILDER_H
