//
// Created by dantas on 03/04/21.
//

#include "control_configuration_parser.h"


MetadataContainer* ControlConfigurationParser::parseMetadataContainer(YAML::Node dp_configs){
    MetadataContainer* mdc;
    YAML::Node metadata_container_configs = dp_configs["metadata_container"];

    YAML::Node hierarchy = dp_configs["data_plane"]["hierarchy"];
    int hierarchy_size = hierarchy.size();
    auto source_dir = hierarchy[hierarchy_size - 1]["prefix"].as<std::string>();

    auto regex = metadata_container_configs["regex"].as<std::string>();
    auto e = metadata_container_configs["epochs"];

    auto shuffling = metadata_container_configs["shuffling"].as<std::string>();

    if (e){
        auto epochs =  e.as<int>();
        mdc = new MetadataContainer(source_dir, regex, hierarchy_size, epochs);
        if(shuffling == "true")
            mdc->shuffling_enabled = true;
    }
    else
        std::cerr << "Not enough parameters provided for metadata container build" << std::endl;

    return mdc;
}


ControllerServiceImpl* ControlConfigurationParser::parse(const std::string& file_path){
    YAML::Node configs = YAML::LoadFile(file_path);
    YAML::Node controller_configs = configs["control_plane"];

    auto server_addr = controller_configs["server_address"].as<std::string>();
    auto dp_configs_path = controller_configs["data_plane"]["config_location"].as<std::string>();
    auto periodicity = controller_configs["periodicity"].as<int>();

    YAML::Node dp_configs = YAML::LoadFile(dp_configs_path);
    auto* mc = parseMetadataContainer(dp_configs);

    return new ControllerServiceImpl(periodicity, dp_configs_path, server_addr, mc);
}