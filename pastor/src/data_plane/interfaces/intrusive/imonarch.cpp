//
// Created by dantas on 27/11/20.
//
#include <iostream>
#include <cstdlib>

#include "imonarch.h"
#include "../../logic/parser/configuration_parser.h"

#define CONFIGS_PATH "MONARCH_CONFIGS_PATH"

IMonarch* IMonarch::instance{nullptr};
absl::once_flag IMonarch::once_;

IMonarch *IMonarch::get_instance(){
    absl::call_once(once_, [](){
        instance = new IMonarch();
        instance->standalone_init();;
        instance->start();
    });
    return instance;
}

#if defined(INCLUDE_GRPC)
IMonarch *IMonarch::get_instance(const std::string &c_server_addr){
    return get_instance(0, "none", c_server_addr, "");
}


IMonarch *IMonarch::get_instance(int rank, const std::string &group, const std::string &c_server_addr, const std::string &dp_server_addr){
    absl::call_once(once_, [rank, group, c_server_addr, dp_server_addr](){
        instance = new IMonarch(rank, 0);
        instance->server_attach_init(group, c_server_addr, dp_server_addr);
        instance->start();
    });
    return instance;
}

void IMonarch::server_attach_init(const std::string &group, const std::string &c_server_addr, const std::string &dp_server_addr){
    remote_handler = new RemoteStageBuilder(group, c_server_addr, dp_server_addr);
    data_plane = ConfigurationParser::parse(remote_handler);
    data_plane->set_distributed_params(rank_, worker_id_);
    data_plane->init(false);
    remote_handler->synchronize();
    data_plane->print();
    delete remote_handler;
}
#endif

IMonarch::~IMonarch(){
    delete data_plane;
}

IMonarch::IMonarch(){
    rank_ = 0;
    worker_id_ = 0;
}


IMonarch::IMonarch(int rank, int worker_id){
    rank_ = rank;
    worker_id_ = worker_id;
}

void IMonarch::standalone_init() {
    //TODO put error if path doesn't exist
    data_plane = ConfigurationParser::parse(std::getenv(CONFIGS_PATH));
    data_plane->init(false);
    data_plane->print();
}

void IMonarch::start() {
    data_plane->start();
}

absl::string_view IMonarch::decode_filename(absl::string_view full_path){
    return data_plane->decode_filename(full_path);
}

ssize_t IMonarch::read(const std::string &filename, char* result, uint64_t offset, size_t n){
    return data_plane->read(filename, result, offset, n);
}

ssize_t IMonarch::read_from_id(int file_id, char* result, uint64_t offset, size_t n){
    return data_plane->read_from_id(file_id, result, offset, n);
}

ssize_t IMonarch::read_from_id(int file_id, char* result){
    return data_plane->read_from_id(file_id, result);
}

int IMonarch::get_target_class_from_id(int id){
    return data_plane->get_target_class_from_id(id);
}

int IMonarch::get_target_class(const std::string &filename){
    return data_plane->get_target_class(filename);
}

size_t IMonarch::get_file_size(const std::string &filename){
    return data_plane->get_file_size(filename);
}

size_t IMonarch::get_file_size_from_id(int id){
    return data_plane->get_file_size_from_id(id);
}

int IMonarch::get_file_count(){
    return data_plane->get_file_count();
}