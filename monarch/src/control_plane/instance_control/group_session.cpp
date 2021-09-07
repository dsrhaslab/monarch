//
// Created by dantas on 27/03/21.
//

#include <iostream>
#include "group_session.h"

GroupSession::GroupSession(int world_size_, int number_of_workers_, const std::string& name) {
    GroupSession::world_size = world_size_ > 0 ? world_size_ : 1;
    GroupSession::number_of_workers = number_of_workers_ > 0 ? number_of_workers_ : 1;
    GroupSession::expected_instances = world_size * number_of_workers;
    instances.reserve(expected_instances);
    GroupSession::instances_in_sync = 0;
    GroupSession::started_instances_count = 0;
    GroupSession::name = name;
}

std::tuple<int, int, int>  GroupSession::register_instance(const std::string& server_addr) {
    if(started_instances_count < expected_instances){
        instances.push_back(new DPInstanceSession(started_instances_count, server_addr));
        //returns id [0, started_instances_count -1]
        return {started_instances_count++, world_size, number_of_workers};
    }else
        std::cerr << "Unexpected instance arrived at: " << name << std::endl;
    return {-1, world_size, number_of_workers};
}

bool GroupSession::sync(SynchronizeCall* callback){
    if(instances_in_sync < expected_instances) {
        delayed_responses.push_back(callback);
        return ++instances_in_sync == expected_instances;
    }
    return false;
}