//
// Created by dantas on 25/10/20.
//

#ifndef MONARCHH_CONFIGURATION_PARSER_H
#define MONARCHH_CONFIGURATION_PARSER_H

#include <string>
#include <istream>

#ifdef BAZEL_BUILD
#include <yaml-cpp/yaml.h>
#else
#include "yaml-cpp/yaml.h"
#endif

#include "../stages/monarch.h"
#include "../stages/hierarchical/storage_drivers/storage_driver_builder.h"
#include "../handlers/control_handler_builder.h"
#include "../utils/logging/singleton_logger.h"

class RemoteStageBuilder;

class ConfigurationParser {
private:
    //static void parse_rate_limiter(HierarchicalStageBuilder* builder, YAML::Node root_configs);

    //static void parse_storage_matrix(HierarchicalStageBuilder* builder, YAML::Node root_configs, int world_size, int num_workers);

    //static PrefetchDataPlaneBuilder* parse_prefetch_stage_builder(HierarchicalStage *root_data_plane, YAML::Node configs);

    #if defined(INCLUDE_GRPC)
    static HierarchicalStageBuilder* parseHierarchicalDataPlaneBuilder(RemoteStageBuilder* rbuilder, YAML::Node root_data_plane_configs);
    #endif

    static MetadataContainerService* parse_metadata_container_service(YAML::Node data_plane_configs);

    static ReadPolicy parse_read_policy(YAML::Node handlers_configs);

    static ControlPolicy parse_control_policy(YAML::Node handlers_configs);

    static EvictCallPolicy parse_evict_call_type(YAML::Node handlers_configs);

    static PlacementPolicy parse_placement_policy(YAML::Node handlers_configs);

    static ControlHandler* parse_control_handler(YAML::Node handlers_configs);

    static StorageDriverType parse_storage_driver_type(YAML::Node driver_configs);

    static StorageDriverSubType parse_storage_driver_subtype(YAML::Node driver_configs);

    static StorageDriverBuilder* parse_posix_file_system_driver(YAML::Node driver_configs);

    static StorageDriver* parse_storage_driver(YAML::Node driver_configs, int level);

    static void parse_storage_hierarchy(HierarchicalStageBuilder* builder, YAML::Node hierarchical_stage_configs);

    static void parse_thread_pool(HierarchicalStageBuilder* builder, YAML::Node hierarchical_stage_configs);

    static HierarchicalStage* parse_hierarchical_stage(YAML::Node hierarchical_stage_configs);

    static Logger* parse_debug_logger(YAML::Node configs, int unique_id);

    static ProfilingService* parse_profiling_service(YAML::Node configs, ProfilerProxy* pp);

    static ProfilerProxy* parse_profiler(YAML::Node configs);

    static void create_workspace(YAML::Node configs);

public:
    #if defined(INCLUDE_GRPC)
    static Stage* parse(RemoteStageBuilder* rbuilder);
    #endif
    static Monarch* parse(const std::string &configs_path);
};

#endif //MONARCHH_CONFIGURATION_PARSER_H
