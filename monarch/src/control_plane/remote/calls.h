//
// Created by dantas on 27/03/21.
//

#ifndef THESIS_CALLS_H
#define THESIS_CALLS_H

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include <grpcpp/alarm.h>

#include "async_server.h"
#include "../controller_service_impl.h"
#ifdef BAZEL_BUILD
#include "protos/controller_service.grpc.pb.h"
#elif TF_BAZEL_BUILD
#include "tensorflow/core/platform/pastor/protos/controller_service.grpc.pb.h"
#else
#include "controller_service.grpc.pb.h"
#endif

using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::ServerAsyncResponseWriter;
using grpc::Status;

using controllerservice::SessionCreated;
using controllerservice::DataSourceInfo;
using controllerservice::SessionInfo;
using controllerservice::RegisterRequest;
using controllerservice::Configuration;
using controllerservice::InstanceInfo;
using controllerservice::TargetInfo;
using controllerservice::ControllerMetadataContainer;
using controllerservice::ControllerFileInfo;
using controllerservice::Empty;
using controllerservice::Controller;


struct CallData {
    //AsyncServer* server; -> only needed if we need to stop requests not only on a big initial init
    Controller::AsyncService* service;
    grpc::ServerCompletionQueue* cq;
    ControllerServiceImpl* controller_service_impl_;
};

// Base class used to cast the void* tags we get from
// the completion queue and call Proceed() on them.
class Call {
protected:
    CallData* data_;
    grpc::ServerContext ctx_;
    enum CallStatus { PROCESS, FINISH };
    CallStatus status_;
public:
    explicit Call( CallData* data) : data_(data), status_{PROCESS} {}
    virtual void Proceed(bool ok) = 0;
    virtual void Stop() = 0;
};

class RegisterSessionCall final : public Call {
private:
    grpc::ServerAsyncResponseWriter<SessionCreated> responder_;
    SessionInfo request;
    SessionCreated reply;
public:
    explicit RegisterSessionCall(CallData* data) : Call(data), responder_(&ctx_) {
        data->service->RequestRegisterSession(&ctx_, &request, &responder_, data_->cq, data_->cq, this);
    }

    void Proceed(bool ok) override{
        switch (status_) {
            case PROCESS: {
                //Spawn new instance to serve new requests
                new RegisterSessionCall(data_);
                std::cout << "Received RegisterSession request" << std::endl;
                std::string group_name = data_->controller_service_impl_->register_group(
                        request.world_size(), request.number_of_workers(),
                        request.instance_type(), request.application_id());

                reply.set_group(group_name);
                auto *metadata_container = data_->controller_service_impl_->metadata_container;

                for (auto &pair : metadata_container->get_data_source_infos()) {
                    DataSourceInfo *info = reply.add_data_source_infos();
                    info->set_name(std::get<0>(pair));
                    info->set_sample_count(std::get<1>(pair));
                }

                if (metadata_container->shuffling_enabled)
                    for (auto seed : metadata_container->shuffling_seeds)
                        reply.add_random_seeds(seed);

                reply.set_epochs(metadata_container->epochs);

                if (request.return_names()) {
                    for (auto &entry : metadata_container->infos) {
                        reply.add_filenames(entry->name());
                    }
                    reply.set_data_source_full_path(metadata_container->data_dir);
                }

                std::cout << "Finished RegisterSession request" << std::endl;
                status_ = FINISH;
                responder_.Finish(reply, Status::OK, this);
                break;
            }
            case FINISH:
                std::cout << "RegisterSession RPC finished " << std::endl;
                delete this;
                break;
        }
    }

    void Stop(){
        std::cerr << "Finishing up client. Unexpected error." << std::endl;
        status_ = FINISH;
    }
};

class RegisterInstanceCall final : public Call {
private:
    grpc::ServerAsyncResponseWriter<Configuration> responder_;
    RegisterRequest request;
    Configuration reply;
public:
    explicit RegisterInstanceCall(CallData* data)  : Call(data), responder_(&ctx_) {
        data->service->RequestRegisterInstance(&ctx_, &request, &responder_, data_->cq, data_->cq, this);
    }

    std::unique_ptr<ControllerMetadataContainer> build_controller_metadata_container(MetadataContainer* metadata_container){
        auto* cmc = new ControllerMetadataContainer();

        cmc->set_full_size(metadata_container->full_size);
        cmc->set_epochs(metadata_container->epochs);
        cmc->set_file_count(metadata_container->file_count);
        cmc->set_storage_source_level(metadata_container->storage_source_level);
        cmc->set_shuffling_enabled(metadata_container->shuffling_enabled);

        for(auto& entry : metadata_container->target_class_to_id){
            TargetInfo* target_info = cmc->add_targets_info();
            target_info->set_id(entry.second);
            target_info->set_name(entry.first);
        }

        for(auto& pair : metadata_container->get_data_source_infos()){
            DataSourceInfo* info = cmc->add_data_source_infos();
            info->set_name(std::get<0>(pair));
            info->set_sample_count(std::get<1>(pair));
        }

        for(int seed : metadata_container->shuffling_seeds)
            cmc->add_random_seeds(seed);

        for(auto* controller_file_info : metadata_container->infos){
            ControllerFileInfo* cfi = cmc->add_file_infos();
            cfi->set_id(controller_file_info->id());
            cfi->set_name(controller_file_info->name());
            cfi->set_size(controller_file_info->size());
            cfi->set_target(controller_file_info->target());
        }

        return std::unique_ptr<ControllerMetadataContainer>(cmc);
    }

    void Proceed(bool ok) override{
        switch (status_) {
            case PROCESS: {
                //Spawn new instance to serve new requests
                new RegisterInstanceCall(data_);

                std::cout << "Received RegisterInstance request. Group: " << request.group() << std::endl;

                auto group_info = data_->controller_service_impl_->register_instance(request.group(), request.data_plane_server_addr());
                reply.set_id(std::get<0>(group_info));
                reply.set_world_size(std::get<1>(group_info));
                reply.set_number_of_workers(std::get<2>(group_info));
                reply.set_configuration_file(data_->controller_service_impl_->get_data_plane_configuration());

                auto cmc = build_controller_metadata_container(data_->controller_service_impl_->metadata_container);
                reply.mutable_metadata()->CopyFrom(*cmc);

                std::cout << "Finished RegisterInstance request" << std::endl;
                status_ = FINISH;
                responder_.Finish(reply, Status::OK, this);
                break;
            }
            case FINISH:
                std::cout << "RegisterInstance RPC finished" << std::endl;
                delete this;
                break;
        }
    }

    void Stop(){
        std::cerr << "Finishing up client. Unexpected error." << std::endl;
        status_ = FINISH;
    }

};

class SynchronizeCall final : public Call {
private:
    grpc::ServerAsyncResponseWriter<Empty> responder_;
    InstanceInfo request;
    Empty reply;
    bool delayed = false;
public:
    explicit SynchronizeCall(CallData* data) : Call(data), responder_(&ctx_) {
        data->service->RequestSynchronize(&ctx_, &request, &responder_, data_->cq, data_->cq, this);
    }

    void Proceed(bool ok) override{
        switch (status_) {
            case PROCESS:
                if(!delayed){
                    //Spawn new instance to serve new requests
                    new SynchronizeCall(data_);
                    std::cout << "Received Synchronize request. Instance: " << request.id() << std::endl;
                    delayed = true;
                    if(request.group() == "none"){
                        status_ = FINISH;
                        responder_.Finish(reply, Status::OK, this);
                    }
                    else if(data_->controller_service_impl_->sync_instance(request.group(), this)) {
                        std::cout << "All instances synchronized. Answering requests" << std::endl;
                        for (auto &call : data_->controller_service_impl_->get_delayed_responses(request.group()))
                            call->Proceed(true);
                    }
                }
                else{
                    status_ = FINISH;
                    responder_.Finish(reply, Status::OK, this);
                }
                break;
            case FINISH:
                delete this;
                break;
        }
    }

    void Stop(){
        std::cerr << "Finishing up client. Unexpected error." << std::endl;
        status_ = FINISH;
    }
};

#endif //THESIS_CALLS_H
