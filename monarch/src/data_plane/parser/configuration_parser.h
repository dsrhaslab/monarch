//
// Created by dantas on 25/10/20.
//

#ifndef THESIS_CONFIGURATION_PARSER_H
#define THESIS_CONFIGURATION_PARSER_H

#include <string>
#include <istream>
#include "../data_plane.h"
#include "../root/hierarchical_data_plane_builder.h"
#include "../prefetching/prefetch_data_plane_builder.h"

#ifdef BAZEL_BUILD
#include <yaml-cpp/yaml.h>
#else
#include "yaml-cpp/yaml.h"
#endif

class RemoteStageBuilder;

class ConfigurationParser {
private:
    static ControlPolicy parse_control_policy(YAML::Node type_configs);
    static ReadPolicy parse_read_policy(YAML::Node type_configs);
    static PlacementPolicy parse_placement_policy(YAML::Node type_configs);
    static EvictCallType parse_evict_call_type(YAML::Node type_configs);
    static HierarchicalDataPlaneBuilder* parseHierarchicalDataPlaneBuilder(RemoteStageBuilder* rbuilder, YAML::Node root_data_plane_configs);
    static PrefetchDataPlaneBuilder* parsePrefetchDataPlaneBuilder(HierarchicalDataPlane *root_data_plane, YAML::Node configs);
    static DataPlane* parseDataPlane(HierarchicalDataPlaneBuilder* root_data_plane_builder, YAML::Node configs);
    static DataStorageDriver* parseStorageDriver(YAML::Node driver_configs, YAML::Node configs, int level);
    static ProfilerProxy* parseProfiler(YAML::Node configs);
public:
    static DataPlane* parse(RemoteStageBuilder* rbuilder);
};

#endif //THESIS_CONFIGURATION_PARSER_H
