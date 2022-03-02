//
// Created by dantas on 26/03/21.
//

#if !defined(THESIS_BOOTSTRAP_CLIENT_H) && defined(INCLUDE_GRPC)
#define THESIS_BOOTSTRAP_CLIENT_H

#include <iostream>
#include <grpcpp/grpcpp.h>
#include <vector>
#include <tuple>

#include "../metadata/metadata_container_service.h"
#include "../metadata/list_transforms.h"
#ifdef BAZEL_BUILD
#include "protos/controller_service.grpc.pb.h"
#elif TF_BAZEL_BUILD
#include "tensorflow/core/platform/pastor/protos/controller_service.grpc.pb.h"
#else
#include "controller_service.grpc.pb.h"
#endif

using grpc::Channel;
using grpc::ClientContext;

using controllerservice::SessionInfo;
using controllerservice::SessionCreated;
using controllerservice::Controller;

class BootstrapClient {
    std::unique_ptr<Controller::Stub> stub_;
    std::string group;
    std::vector<std::tuple<std::string, int>> partitioned_data_source_infos;
    std::vector<std::tuple<std::string, int>> data_source_infos;
    std::vector<int> shuffling_seeds;
    std::vector<std::string> filenames;
    int world_size_;
    int number_of_workers_;
    int epochs = 0;
    std::string data_source_full_path;


public:
    BootstrapClient(const std::string& c_server_addr)
    : stub_(Controller::NewStub(grpc::CreateChannel(c_server_addr,grpc::InsecureChannelCredentials()))) {}

    void request_session(const std::string& type, const std::string& application_id, int world_size, int number_of_workers, bool return_names){
        world_size_ = world_size;
        number_of_workers_ = number_of_workers;

        SessionInfo request;
        request.set_instance_type(type);
        request.set_application_id(application_id);
        request.set_world_size(world_size);
        request.set_number_of_workers(number_of_workers);
        request.set_return_names(return_names);

        SessionCreated reply;

        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        // The actual RPC.
        grpc::Status status = stub_->RegisterSession(&context, request, &reply);

        // Act upon its status.
        if (status.ok()) {
            group = reply.group();
            for(const auto& s : reply.data_source_infos()) {
                int count = s.sample_count();
                auto sizes_info = ListTransforms::get_sizes(count, world_size_);
                partitioned_data_source_infos.emplace_back(s.name(), std::get<0>(sizes_info));
                data_source_infos.emplace_back(s.name(), count);
            }
            if(!reply.random_seeds().empty())
                for(auto seed : reply.random_seeds())
                    shuffling_seeds.push_back(seed);
            epochs = reply.epochs();

            if(return_names){
                filenames.reserve(reply.filenames().size());
                for(const auto& filename : reply.filenames())
                    filenames.push_back(filename);
                data_source_full_path = reply.data_source_full_path();
            }
        } else {
            std::cerr << status.error_code() << ": " << status.error_message()
                      << std::endl;
        }
    }

    void simple_request_session(const std::string& type, int world_size, int number_of_workers, bool return_names){
        request_session(type, "none", world_size, number_of_workers, return_names);
    }

    std::string get_group() {
        if(group != "")
            return group;
        std::cerr << "No group defined. RequestSession must be called first" << std::endl;
        return "";
    }

    std::vector<std::tuple<std::string, int>> get_data_source_infos() {
        if(data_source_infos.empty())
            std::cerr << "No data source infos defined. RequestSession must be called first" << std::endl;
        return partitioned_data_source_infos;
    }

    //data_source_infos.size() == 1 is only dealt f this case
    std::vector<std::string> get_filenames(){
        if(filenames.empty()) {
            std::cout << "Filenames are empty Returning empty list" << std::endl;
            return std::vector<std::string>();
        }
        if(!shuffling_seeds.empty())
            return ListTransforms::make_shuffled_list(data_source_infos, filenames, shuffling_seeds);
        else {
            if(epochs == 0)
                std::cerr << "No epochs number defined. RequestSession must be called first" << std::endl;
            return ListTransforms::make_list(data_source_infos, filenames, epochs);
        }
    }

    std::vector<std::string> get_filenames_full_path(){
        if(filenames.empty()) {
            std::cout << "Filenames are empty Returning empty list" << std::endl;
            return std::vector<std::string>();
        }
        std::vector<std::string> full_paths;
        for(auto& filename : filenames) {
            full_paths.push_back(data_source_full_path + "/" + filename);
        }
        if(!shuffling_seeds.empty())
            return ListTransforms::make_shuffled_list(data_source_infos,full_paths, shuffling_seeds);
        else {
            if(epochs == 0)
                std::cerr << "No epochs number defined. RequestSession must be called first" << std::endl;
            return ListTransforms::make_list(data_source_infos, full_paths, epochs);
        }
    }

    std::vector<int> get_ids(){
        if(!shuffling_seeds.empty())
            return ListTransforms::make_shuffled_list(-1, 1, data_source_infos, shuffling_seeds);
        else {
            if(epochs == 0)
                std::cerr << "No epochs number defined. RequestSession must be called first" << std::endl;
            return ListTransforms::make_list(-1, 1, data_source_infos, epochs);
        }
    }

    std::vector<int> get_ids_from_rank(int rank){
        if(!shuffling_seeds.empty())
            return ListTransforms::make_shuffled_list(rank, world_size_, data_source_infos, shuffling_seeds);
        else {
            if(epochs == 0)
                std::cerr << "No epochs number defined. RequestSession must be called first" << std::endl;
            return ListTransforms::make_list(rank, world_size_, data_source_infos, epochs);
        }
    }
};

#endif //THESIS_BOOTSTRAP_CLIENT_H
