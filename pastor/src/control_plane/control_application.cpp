//
// Created by dantas on 28/02/21.
//
#include <iostream>
#include <thread>

#include "control_application.h"
#include "../control_plane/parser/control_configuration_parser.h"
#include "remote/async_server.h"

ControlApplication::ControlApplication(){
    server = new AsyncServer();
}

void ControlApplication::build_metadata_container(){
    std::cout << "Starting metadata container parse" << std::endl;
    service->metadata_container->parse();
    for(auto& str : service->metadata_container->configs())
        std::cout << str;
    server->set_ready();
}

void ControlApplication::run(const std::string& configs_file_path){
    service = ControlConfigurationParser::parse(configs_file_path);
    std::thread requests_thread(&ControlApplication::build_metadata_container, this);
    requests_thread.detach();
    server->run(service);
}


