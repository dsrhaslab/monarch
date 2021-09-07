//
// Created by dantas on 27/11/20.
//
#include <iostream>

#include "data_plane_instance.h"
#include "parser/configuration_parser.h"

DataPlaneInstance* DataPlaneInstance::instance{nullptr};
absl::once_flag DataPlaneInstance::once_;

DataPlaneInstance *DataPlaneInstance::get_instance(const std::string &c_server_addr){
    return get_instance(0, "none", c_server_addr, "");
}


DataPlaneInstance *DataPlaneInstance::get_instance(int rank, const std::string &group, const std::string &c_server_addr, const std::string &dp_server_addr){
    absl::call_once(once_, [rank, group, c_server_addr, dp_server_addr](){
        instance = new DataPlaneInstance(rank, 0);
        instance->bind(group, c_server_addr, dp_server_addr);
        instance->start();
    });
    return instance;
}


DataPlaneInstance::DataPlaneInstance(){
    rank_ = 0;
    worker_id_ = 0;
}

DataPlaneInstance::DataPlaneInstance(int rank, int worker_id){
    rank_ = rank;
    worker_id_ = worker_id;
}

void DataPlaneInstance::bind(const std::string &group, const std::string &c_server_addr, const std::string &dp_server_addr){
    remote_handler = new RemoteStageBuilder(group, c_server_addr, dp_server_addr);
    data_plane = ConfigurationParser::parse(remote_handler);
    data_plane->set_distributed_params(rank_, worker_id_);
    data_plane->init();
    remote_handler->synchronize();
    data_plane->print();
    delete remote_handler;
}

void DataPlaneInstance::start() {
    data_plane->start();
}

std::string DataPlaneInstance::decode_filename(std::string full_path){
    return data_plane->decode_filename(full_path);
}

ssize_t DataPlaneInstance::read(const std::string &filename, char* result, uint64_t offset, size_t n){
    return data_plane->read(filename, result, offset, n);
}

ssize_t DataPlaneInstance::read_from_id(int file_id, char* result, uint64_t offset, size_t n){
    return data_plane->read_from_id(file_id, result, offset, n);
}

ssize_t DataPlaneInstance::read_from_id(int file_id, char* result){
    return data_plane->read_from_id(file_id, result);
}

int DataPlaneInstance::get_target_class_from_id(int id){
    return data_plane->get_target_class_from_id(id);
}

int DataPlaneInstance::get_target_class(const std::string &filename){
    return data_plane->get_target_class(filename);
}

size_t DataPlaneInstance::get_file_size(const std::string &filename){
    return data_plane->get_file_size(filename);
}

size_t DataPlaneInstance::get_file_size_from_id(int id){
    return data_plane->get_file_size_from_id(id);
}

int DataPlaneInstance::get_file_count(){
    return data_plane->get_file_count();
}

CollectedStats* DataPlaneInstance::collect_statistics(){
    return data_plane->collect_statistics();
}