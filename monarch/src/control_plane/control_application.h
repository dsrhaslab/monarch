//
// Created by dantas on 28/02/21.
//

#ifndef MONARCH_CONTROL_APPLICATION_H
#define MONARCH_CONTROL_APPLICATION_H

#include <unordered_map>
#include "controller_service_impl.h"

class AsyncServer;

class ControlApplication {
    AsyncServer* server;
    ControllerServiceImpl* service;

    void build_metadata_container();

public:
    ControlApplication();
    void run(const std::string& configs_file_path);
};


#endif //MONARCH_CONTROL_APPLICATION_H
