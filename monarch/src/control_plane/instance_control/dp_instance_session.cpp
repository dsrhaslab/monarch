//
// Created by dantas on 28/02/21.
//

#include "dp_instance_session.h"
#include <iostream>
#include <unistd.h>
#include <sys/un.h>

DPInstanceSession::DPInstanceSession(int id, const std::string& server_addr){
    DPInstanceSession::id = id;
    DPInstanceSession::server_addr = server_addr;
}

int DPInstanceSession::get_id() const{
    return id;
}