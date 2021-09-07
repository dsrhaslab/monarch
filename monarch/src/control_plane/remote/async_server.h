//
// Created by dantas on 27/03/21.
//

#ifndef THESIS_REQUEST_HANDLER_H
#define THESIS_REQUEST_HANDLER_H

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include <atomic>
#include <mutex>

#ifdef BAZEL_BUILD
#include "protos/controller_service.grpc.pb.h"
#elif TF_BAZEL_BUILD
#include "tensorflow/core/platform/pastor/protos/controller_service.grpc.pb.h"
#else
#include "controller_service.grpc.pb.h"
#endif

class ControlApplication;
class ControllerServiceImpl;
class Call;

using grpc::Server;
using grpc::ServerCompletionQueue;
using controllerservice::Controller;

class AsyncServer {
    std::unique_ptr<Server> server_;
    std::unique_ptr<ServerCompletionQueue> cq_;
    Controller::AsyncService service_;
    ControllerServiceImpl* controller_service_impl_;

    std::mutex delayed_mutex;
    std::vector<Call*> delayed_responses;
    std::atomic<bool> ready {false};

    void HandleRpcs();
    void flush_delayed_responses();

public:
    ~AsyncServer();
    void run(ControllerServiceImpl* controller_service_impl);
    void set_ready();
};

#endif //THESIS_REQUEST_HANDLER_H
