//
// Created by dantas on 27/03/21.
//

#include "calls.h"
#include <thread>

using grpc::ServerBuilder;

AsyncServer::~AsyncServer() {
    server_->Shutdown();
    // Always shutdown the completion queue after the server.
    cq_->Shutdown();
}

void AsyncServer::run(ControllerServiceImpl* controller_service_impl) {
    controller_service_impl_ = controller_service_impl;
    ServerBuilder builder;

    // Listen on the given address without any authentication mechanism.
    int selected_port;
    builder.AddListeningPort(controller_service_impl_->server_address, grpc::InsecureServerCredentials(), &selected_port);
    // Register "service_" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *asynchronous* service.
    builder.RegisterService(&service_);
    // Get hold of the completion queue used for the asynchronous communication
    // with the gRPC runtime.
    cq_ = builder.AddCompletionQueue();
    // Finally assemble the server.
    server_ = builder.BuildAndStart();
    if (selected_port == 0) {
        std::cerr << "Could not bind to a port" << std::endl;
        exit(EXIT_FAILURE);
    }else {
        std::cout << "Server listening on port " << selected_port << std::endl;
    }
    // Proceed to the server's main loop.
    HandleRpcs();
}

void AsyncServer::set_ready(){
    std::cout << "Server ready to process rpcs" << std::endl;
    ready = true;
    std::unique_lock<std::mutex> ul(delayed_mutex);
    flush_delayed_responses();
}

//https://github.com/grpc/grpc/issues/15535 -> multi-channels
//https://www.gresearch.co.uk/article/lessons-learnt-from-writing-asynchronous-streaming-grpc-services-in-c/
//https://www.programmersought.com/article/42685277702/
void AsyncServer::HandleRpcs() {
    // Spawn all Call instances to serve new clients.
    CallData data{&service_, cq_.get(), controller_service_impl_};
    new RegisterSessionCall(&data);
    new RegisterInstanceCall(&data);
    new SynchronizeCall(&data);

    void* tag;  // uniquely identifies a request.
    bool ok;
    //TODO use stop method
    //https://github.com/G-Research/grpc_async_examples/blob/master/async_streaming_server_queue_to_front.cc
    while (true) {
        // For the meaning of the return value of Next, and ok see:
        // https://groups.google.com/d/msg/grpc-io/qtZya6AuGAQ/Umepla-GAAAJ
        // http://www.grpc.io/grpc/cpp/classgrpc_1_1_completion_queue.html
        GPR_ASSERT(cq_->Next(&tag, &ok));
        if(ok){
            auto* call = static_cast<Call*>(tag);
            if(ready)
                call->Proceed(ok);
            else {
                std::unique_lock<std::mutex> ul(delayed_mutex);
                delayed_responses.push_back(call);
            }
        }
        else{
            static_cast<Call*>(tag)->Stop();
            continue;
        }
    }
}

//TODO fix -> Ignores ok value
void AsyncServer::flush_delayed_responses() {
    std::cout << "Processing all delayed rpcs. " << delayed_responses.size() << " requests were delayed" << std::endl;
    for(auto& call : delayed_responses)
        call->Proceed(true);
}
