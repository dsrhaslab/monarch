//
// Created by dantas on 28/02/21.
//
#if defined(INCLUDE_GRPC)
#include <iostream>

#include "../metadata/strict_file_info.h"
#include "../metadata/placed_file_info.h"
#include "remote_stage_builder.h"

using grpc::ChannelArguments;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientReader;

using controllerservice::InstanceInfo;
using controllerservice::RegisterRequest;
using controllerservice::ControllerFileInfo;
using controllerservice::ControllerMetadataContainer;
using controllerservice::TargetInfo;
using controllerservice::Empty;

RemoteStageBuilder::RemoteStageBuilder(const std::string &group, const std::string &c_server_addr, const std::string &dp_server_addr)
  : stub_(Controller::NewStub(get_channel(c_server_addr))) {
    RemoteStageBuilder::group = group;
    RemoteStageBuilder::instance_identifier = -1;
    RemoteStageBuilder::dp_server_addr = dp_server_addr;
    RemoteStageBuilder::metadata_container = new MetadataContainerService<FileInfo>();
}

std::shared_ptr<Channel> RemoteStageBuilder::get_channel(const std::string &c_server_addr) {
    ChannelArguments args;
    args.SetMaxReceiveMessageSize(1024 * 1024 * 2000);
    return grpc::CreateCustomChannel(c_server_addr, grpc::InsecureChannelCredentials(), args);
}

std::string RemoteStageBuilder::get_configuration() {
    RegisterRequest request;
    //not yet defined (-1)
    request.set_data_plane_server_addr(dp_server_addr);
    request.set_group(group);

    ClientContext context;
    Status status = stub_->RegisterInstance(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
        instance_identifier = reply.id();
        world_size = reply.world_size();
        number_of_workers = reply.number_of_workers();
        build_partial_metadata_container(reply);
        std::cout << "Instance " << instance_identifier << " first build step completed. Configuration parsed" << std::endl;
        return reply.configuration_file();
    } else {
        std::cerr << status.error_code() << ": " << status.error_message()
                  << std::endl;
        exit(1);
    }
}

void RemoteStageBuilder::build_partial_metadata_container(Configuration& reply) {
    auto &metadata = reply.metadata();

    metadata_container->set_full_size(metadata.full_size());
    metadata_container->set_world_size(world_size);
    metadata_container->set_distributed_id(instance_identifier);
    metadata_container->set_file_count(metadata.file_count());
    metadata_container->set_epochs(metadata.epochs());
    metadata_container->set_storage_source_level(metadata.storage_source_level());

    storage_source_level = metadata.storage_source_level();

    for (auto &target_info : metadata.targets_info())
        metadata_container->add_target_class_to_id(target_info.name(), target_info.id());

    for (auto &data_source_info :  metadata.data_source_infos())
        metadata_container->add_dir_file_count(data_source_info.name(), data_source_info.sample_count());

    if (metadata.shuffling_enabled()) {
        std::vector<int> seeds;
        seeds.reserve(metadata.random_seeds().size());
        for (auto seed : metadata.random_seeds())
            seeds.push_back(seed);
        metadata_container->generate_samples_ordered_ids(seeds);
    } else
        metadata_container->generate_samples_ordered_ids();
}

MetadataContainerService<FileInfo>* RemoteStageBuilder::get_metadata_container(const std::string& type, bool has_shareable_file_descriptors){
    auto &metadata = reply.metadata();

    for (auto &cfi : metadata.file_infos()) {
        if (type == "root_standalone")
            metadata_container->add_entry(cfi.id(), new PlacedFileInfo(cfi.name(), cfi.size(), storage_source_level, has_shareable_file_descriptors, cfi.target()));
        else
            metadata_container->add_entry(cfi.id(), new StrictFileInfo(cfi.name(), cfi.size(), storage_source_level, has_shareable_file_descriptors, cfi.target()));
    }
    std::cout << "Inserted " << metadata.file_infos().size() << " metadata entries";
    return metadata_container;
}

void RemoteStageBuilder::synchronize(){
    InstanceInfo request;
    //not yet defined (-1)
    request.set_id(instance_identifier);
    request.set_group(group);

    Empty reply;
    ClientContext context;
    Status status = stub_->Synchronize(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
        std::cout << "Instance " << instance_identifier << "synchronized" << std::endl;
    } else {
        std::cerr << status.error_code() << ": " << status.error_message()
                  << std::endl;
        exit(1);
    }
}

int RemoteStageBuilder::get_instance_identifier() {
    return instance_identifier;
}

int RemoteStageBuilder::get_world_size(){
    return world_size;
}

int RemoteStageBuilder::get_number_of_workers(){
    return number_of_workers;
}
#endif