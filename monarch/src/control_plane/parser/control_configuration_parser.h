//
// Created by dantas on 03/04/21.
//

#ifndef MONARCH_CONFIGURATION_PARSER_H
#define MONARCH_CONFIGURATION_PARSER_H

#ifdef BAZEL_BUILD
#include <yaml-cpp/yaml.h>
#else
#include "yaml-cpp/yaml.h"
#endif

#include "../controller_service_impl.h"
#include "../metadata/metadata_container.h"

class ControlConfigurationParser {
    static MetadataContainer* parseMetadataContainer(YAML::Node dp_configs);
public:
    static ControllerServiceImpl* parse(const std::string& file_path);
};


#endif //MONARCH_CONFIGURATION_PARSER_H
