//
// Created by dantas on 27/03/21.
//

#ifndef THESIS_CONTROLLER_SERVICE_IMPL_H
#define THESIS_CONTROLLER_SERVICE_IMPL_H

#include "instance_control/group_session.h"
#include "metadata/metadata_container.h"

class ControllerServiceImpl {
    int periodicity;
    //data plane configuration path
    std::string configs_path;
    int groups;
    std::unordered_map<std::string, GroupSession*> sessions;

public:
    std::string server_address;
    MetadataContainer* metadata_container;

    ControllerServiceImpl(int periodicity, const std::string& configs_path, const std::string& server_addr, MetadataContainer* metadata_container);
    std::string register_group(int world_size, int number_of_workers, const std::string& instance_type, const std::string& application_id);
    std::tuple<int, int, int> register_instance(const std::string& group_name, const std::string& data_plane_server_addr);
    bool sync_instance(const std::string& group_name, SynchronizeCall* callback);
    std::vector<SynchronizeCall*>& get_delayed_responses(const std::string& group_name);
    std::string get_data_plane_configuration();


};


#endif //THESIS_CONTROLLER_SERVICE_IMPL_H
