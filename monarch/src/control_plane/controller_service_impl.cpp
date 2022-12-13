//
// Created by dantas on 27/03/21.
//

#include <iostream>
#include <fstream>
#include "controller_service_impl.h"

ControllerServiceImpl::ControllerServiceImpl(int periodicity, const std::string& configs_path, const std::string& server_addr, MetadataContainer* metadata_container){
    ControllerServiceImpl::periodicity = periodicity;
    ControllerServiceImpl::configs_path = configs_path;
    ControllerServiceImpl::server_address = server_addr;
    ControllerServiceImpl::metadata_container = metadata_container;
    ControllerServiceImpl::groups = 0;
}

std::string ControllerServiceImpl::register_group(int world_size, int number_of_workers, const std::string &instance_type, const std::string &application_id) {
    std::string group_name;
    if(instance_type == "test"){
        group_name = instance_type;
    }else if (instance_type == "single_process"){
        //don't register for now
        return "none";
    }else if (application_id == "none"){
        group_name = instance_type + "-group-" + std::to_string(groups++);
    }else {
        group_name = instance_type + "-group-" + application_id;
        auto entry = sessions.find(group_name);
        if(entry != sessions.end()){
            return group_name;
        }
    }
    std::cout << "New group formed: " << group_name << std::endl;
    sessions.insert(std::make_pair(group_name, new GroupSession(world_size, number_of_workers, group_name)));
    return group_name;
}


std::tuple<int, int, int>  ControllerServiceImpl::register_instance(const std::string& group_name, const std::string& data_plane_server_addr){
    if(group_name == "none")
        return {0, 1, 1};
    auto entry = sessions.find(group_name);
    if(entry != sessions.end()){
        auto res = entry->second->register_instance(data_plane_server_addr);
        std::cout << "New instance registered with id: " << std::get<0>(res) << std::endl;
        return res;
    }else {
        std::cerr << "Register instance request error: No group with the name: " << group_name << std::endl;
        return {-1, -1, -1};
    }
}

bool ControllerServiceImpl::sync_instance(const std::string& group_name, SynchronizeCall* callback){
    auto entry = sessions.find(group_name);
    if(entry != sessions.end()){
        return entry->second->sync(callback);
    }else {
        std::cerr << "Sync Instance request error: No group with the name: " << group_name << std::endl;
        return -1;
    }
}

std::vector<SynchronizeCall*>& ControllerServiceImpl::get_delayed_responses(const std::string& group_name){
    return sessions.find(group_name)->second->delayed_responses;
}

std::string ControllerServiceImpl::get_data_plane_configuration(){
    std::ifstream in(configs_path.c_str());
    return std::move(std::string ((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()));
}
