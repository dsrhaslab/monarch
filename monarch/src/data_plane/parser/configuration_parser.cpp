//
// Created by dantas on 25/10/20.
//

#include <iostream>
#include <cstdio>
#include <sys/stat.h>

#if defined(INCLUDE_GRPC)
#include "../remote/remote_stage_builder.h"
#endif

#include "configuration_parser.h"
#include "../stages/monarch_builder.h"
#include "../stages/hierarchical/storage_drivers/memory_buffers/memory_buffer_driver_builder.h"
#include "../stages/hierarchical/storage_drivers/file_systems/posix/posix_file_system_driver_builder.h"
#include "../stages/hierarchical/hierarchical_stage_builder.h"
#include "../data_governance/services/metadata_container_service_builder.h"

#define DEBUG_DIR "/debugger"
#define PROFILER_DIR "/profiling"

/*
void ConfigurationParser::parse_rate_limiter(HierarchicalStageBuilder* builder, YAML::Node root_configs){
    YAML::Node configs = root_configs["stage"];
    auto r_limiter = configs["rate_limiter"];
    if(r_limiter){
        auto type = r_limiter["type"].as<std::string>();
        if(type == "batch"){
            auto b_size = r_limiter["batch_size"];
            if(!b_size)
                std::cerr << "define a batch size for the batch rate limiter\n";
            else
                builder->with_batch_rate_limit(b_size.as<int>());
        } else if(type == "client_watch"){
            auto b_size = r_limiter["limit_size"];
            if(!b_size)
                std::cerr << "define a limit size for the client watch rate limiter\n";
            else
                builder->with_client_watch_rate_limit(b_size.as<int>());
        }
        else
            std::cerr << "rate limiter type not supported\n";
    }
}
*/

/**
void ConfigurationParser::parse_storage_matrix(HierarchicalStageBuilder* builder, YAML::Node root_configs, int world_size, int num_workers){
    YAML::Node configs = root_configs["stage"];
    YAML::Node hierarchy = configs["hierarchy"];
    std::vector<std::vector<StorageDriver*>> matrix;
    for (int j = 0; j < world_size * num_workers; j++) {
        std::vector<StorageDriver*> drivers;
        matrix.push_back(drivers);
        for(int i = 0; i < hierarchy.size(); i++) {
            auto driver = hierarchy[i];
            matrix[j].push_back(parse_storage_driver(driver, configs, i));
        }
    }
    builder->with_storage_hierarchy(matrix);
}
*/

#if defined(INCLUDE_GRPC)
HierarchicalStageBuilder* ConfigurationParser::parseHierarchicalDataPlaneBuilder(RemoteStageBuilder* rbuilder, YAML::Node root_configs){
    YAML::Node configs = root_configs["stage"];
    YAML::Node hierarchy = configs["hierarchy"];
    HierarchicalStageBuilder* builder = HierarchicalStage::create(rbuilder->get_instance_identifier(),
                                                                          rbuilder->get_world_size(),
                                                                          rbuilder->get_number_of_workers(),
                                                                          hierarchy.size());

    if(configs["storage_sync_timeout"]){
        builder->with_storage_synchronization_timeout(configs["storage_sync_timeout"].as<int>());
    }

    parse_storage_matrix(builder, root_configs, rbuilder->get_world_size(), rbuilder->get_number_of_workers());
    parse_thread_pool(builder, root_configs);
    parse_rate_limiter(builder, root_configs);

    if(root_configs["debug"].as<std::string>() == "true")
        builder->with_debug_enabled(root_configs["home_dir"].as<std::string>() + DEBUG_DIR, rbuilder->get_instance_identifier());

    auto type = configs["type"].as<std::string>();
    auto has_shareable_file_descriptors = false;
    if(configs["has_shareable_file_descriptors"] && configs["has_shareable_file_descriptors"].as<std::string>() == "true") {
        has_shareable_file_descriptors = true;
    }
    builder->with_metadata_container(rbuilder->get_metadata_container(type, has_shareable_file_descriptors));
    builder->with_type(type);
    builder->with_profiling_enabled(parseProfiler(root_configs));
    return builder;
}
#endif

/*
PrefetchDataPlaneBuilder* ConfigurationParser::parse_prefetch_stage_builder(HierarchicalStage *root_stage, YAML::Node configs){
    PrefetchDataPlaneBuilder* builder = PrefetchDataPlane::create(root_stage);

    YAML::Node t_configs = configs["type_configs"];

    if(t_configs["eviction_percentage"])
        builder->with_eviction_percentage(t_configs["eviction_percentage"].as<float>());

    if(t_configs["prefetch_tpool_size"])
        builder->with_prefetch_thread_pool_size(t_configs["prefetch_tpool_size"].as<int>());

    builder->with_read_policy(parse_read_policy(t_configs));

    return builder;
}
*/

MetadataContainerService* ConfigurationParser::parse_metadata_container_service(YAML::Node data_plane_configs){
    YAML::Node hierarchy = data_plane_configs["hierarchical_stage"]["hierarchy"];
    auto source_dir = hierarchy[hierarchy.size()- 1]["prefix"].as<std::string>();
    auto metadata_container_service_builder = MetadataContainerService::create(source_dir, hierarchy.size()- 1);

    YAML::Node metadata_container_service_configs = data_plane_configs["data_governance"]["metadata_container_service"];

    if(metadata_container_service_configs["prefix"]){
        std::string prefix = metadata_container_service_configs["prefix"].as<std::string>();
        metadata_container_service_builder.with_train_files_prefix(prefix);
    }

    YAML::Node metadata_options_configs = data_plane_configs["data_governance"]["metadata_options"];

    if(metadata_options_configs["shared_file_descriptors"] && metadata_options_configs["shared_file_descriptors"].as<bool>()){
        metadata_container_service_builder.with_shared_file_descriptors();
    }

    //TODO change if remote becomes an option
    metadata_container_service_builder.with_local_parse();

    return metadata_container_service_builder.build();
}

ReadPolicy ConfigurationParser::parse_read_policy(YAML::Node handlers_configs){
    if(handlers_configs["read_policy"]){
        std::string strat = handlers_configs["read_policy"].as<std::string>();
        if(strat == "wait_enforced")
            return ReadPolicy::WAIT_ENFORCED;
        else if(strat == "relaxed")
            return ReadPolicy::RELAXED;
        else {
            std::cerr << "Bad read policy\n";
            exit(1);
        }
    }else{
        return ReadPolicy::WAIT_ENFORCED;
    }
}

ControlPolicy ConfigurationParser::parse_control_policy(YAML::Node handlers_configs){
    if(handlers_configs["control_policy"]){
        std::string pol = handlers_configs["control_policy"].as<std::string>();
        if(pol == "lock_ordered")
            return ControlPolicy::LOCK_ORDERED;
        else if(pol == "queue_ordered")
            return ControlPolicy::QUEUE_ORDERED;
        else if(pol== "single_thread_ordered")
            return ControlPolicy::SINGLE_THREAD_ORDERED;
        else if(pol == "solo_placement")
            return ControlPolicy::SOLO_PLACEMENT;
        else {
            std::cerr << "Bad control policy\n";
            exit(1);
        }
    }else{
        return ControlPolicy::LOCK_ORDERED;
    }
}

EvictCallPolicy ConfigurationParser::parse_evict_call_type(YAML::Node handlers_configs){
    if(handlers_configs["evict_call_type"]){
        std::string strat = handlers_configs["evict_call_type"].as<std::string>();
        if(strat == "client")
            return EvictCallPolicy::CLIENT;
        else if(strat == "housekeeper")
            return EvictCallPolicy::HOUSEKEEPER;
        else {
            std::cerr << "Bad evict call type\n";
            exit(1);
        }
    }else{
        return EvictCallPolicy::CLIENT;
    }
}

PlacementPolicy ConfigurationParser::parse_placement_policy(YAML::Node handlers_configs){
    if(handlers_configs["placement_policy"]){
        std::string pol = handlers_configs["placement_policy"].as<std::string>();
        if(pol == "push_down")
            return PlacementPolicy::PUSH_DOWN;
        else if(pol == "first_level_only")
            return PlacementPolicy::FIRST_LEVEL_ONLY;
        else{
            std::cerr << "Bad placement policy\n";
            exit(1);
        }
    }else{
        return PlacementPolicy::PUSH_DOWN;
    }
}

ControlHandler* ConfigurationParser::parse_control_handler(YAML::Node handlers_configs){
    auto control_handler_builder = ControlHandler::create();

    control_handler_builder.with_placement_policy(parse_placement_policy(handlers_configs))
        .with_control_policy(parse_control_policy(handlers_configs));

    if(handlers_configs["async_placement"]){
         auto str = handlers_configs["async_placement"].as<std::string>();
         if(str == "true"){
            control_handler_builder.with_async_placement(true);
         }
         else{
            control_handler_builder.with_async_placement(false);
         }
    }

    if(handlers_configs["dedicated_thread_pool"]){
        auto str = handlers_configs["dedicated_thread_pool"].as<std::string>();
        if(str == "true"){
            control_handler_builder.with_dedicated_thread_pool(true);
        }
        else{
            control_handler_builder.with_dedicated_thread_pool(false);
        }
    }

    return control_handler_builder.build();
}

StorageDriverType ConfigurationParser::parse_storage_driver_type(YAML::Node driver_configs){
    auto type = driver_configs["type"].as<std::string>();
    if(type == "file_system"){
        return StorageDriverType::FILE_SYSTEM;
    }else if(type == "memory_buffer"){
        return StorageDriverType::MEMORY_BUFFER;
    }else{
        std::cerr << "Storage driver type " << type << " doesn't exist" << std::endl;
        exit(1);
    }
}

StorageDriverSubType ConfigurationParser::parse_storage_driver_subtype(YAML::Node driver_configs){
    auto subtype = driver_configs["subtype"].as<std::string>();
    if(subtype == "posix"){
        return StorageDriverSubType::POSIX;
    }else if(subtype == "thread_building_blocks"){
        return StorageDriverSubType::THREAD_BUILDING_BLOCKS;
    }else if (subtype == "parallel_hashmap"){
        return StorageDriverSubType::PARALLEL_HASHMAP;
    }else{
        std::cerr << "Storage driver subtype " << subtype << " doesn't exist" << std::endl;
        exit(1);
    }
}

StorageDriverBuilder* ConfigurationParser::parse_posix_file_system_driver(YAML::Node driver_configs){
    auto* driver_builder = PosixFileSystemDriver::create();
    if(driver_configs["block_size"].as<std::string>() == "max"){
        driver_builder->with_max_block_size();
    }else{
        driver_builder->with_block_size(driver_configs["block_size"].as<int>());
    }
    driver_builder->with_storage_prefix(driver_configs["prefix"].as<std::string>());
    return driver_builder;
}

StorageDriver* ConfigurationParser::parse_storage_driver(YAML::Node driver_configs, int level){
    auto type = parse_storage_driver_type(driver_configs);
    auto subtype = parse_storage_driver_subtype(driver_configs);
    StorageDriverBuilder* driver_builder;
    switch (type)
    {
    case StorageDriverType::FILE_SYSTEM:
        switch (subtype)
        {
        case StorageDriverSubType::POSIX:
            driver_builder = parse_posix_file_system_driver(driver_configs);
            break;
    
        default:
            break;
        }
        break;

    //TODO mismatch not checked here
    case StorageDriverType::MEMORY_BUFFER:
        driver_builder = MemoryBufferDriver::create(subtype);
        break;

    default:
        std::cerr << "Type and subtype mismatch for storage level " << level << std::endl;
        exit(1);
    }
    if(YAML::Node mss = driver_configs["max_storage_size"]){
        driver_builder->with_allocation_capabilities(mss.as<size_t>());
    }
    return driver_builder->build();
}

void ConfigurationParser::parse_storage_hierarchy(HierarchicalStageBuilder* builder, YAML::Node hierarchical_stage_configs){
    YAML::Node hierarchy = hierarchical_stage_configs["hierarchy"];
    std::vector<StorageDriver*> drivers;
    for(int i = 0; i < hierarchy.size(); i++){
        auto driver_configs = hierarchy[i];
        drivers.push_back(parse_storage_driver(driver_configs, i));
    }
    builder->with_storage_hierarchy(drivers);
}


void ConfigurationParser::parse_thread_pool(HierarchicalStageBuilder* builder, YAML::Node hierarchical_stage_configs){
    YAML::Node hierarchy = hierarchical_stage_configs["hierarchy"];
    bool shared_tpool = false;
    auto tpool_size = hierarchical_stage_configs["shared_tpool_size"];

    if(tpool_size){
        shared_tpool = true;
        builder->with_shared_thread_pool(tpool_size.as<int>());
    }

    if (!shared_tpool) {
        std::vector<ctpl::thread_pool*> pools;
        for(int i = 0; i < hierarchy.size(); i++){
            auto driver = hierarchy[i];
            if (!driver["tpool_size"]) {
                std::cerr << "define a thread pool size for every storage level if shared_tpool is not used\n";
                exit(1);
            }
            pools.push_back( new ctpl::thread_pool(driver["tpool_size"].as<int>()));
        }
        builder->with_storage_hierarchy_pools(pools);
    }
}

HierarchicalStage* ConfigurationParser::parse_hierarchical_stage(YAML::Node hierarchical_stage_configs){
    YAML::Node hierarchy = hierarchical_stage_configs["hierarchy"];
    HierarchicalStageBuilder* builder = HierarchicalStage::create(hierarchy.size());
    if(hierarchical_stage_configs["private_debug"] && !hierarchical_stage_configs["private_debug"].as<bool>()) {
        builder->with_private_debug_disabled();
    }
    parse_thread_pool(builder, hierarchical_stage_configs);
    parse_storage_hierarchy(builder, hierarchical_stage_configs);
    return builder->build();
}

Logger* ConfigurationParser::parse_debug_logger(YAML::Node configs, int unique_id){
    return SingletonLogger::create_instance(configs["workspace"].as<std::string>() + DEBUG_DIR, unique_id);
}

ProfilingService* ConfigurationParser::parse_profiling_service(YAML::Node configs, ProfilerProxy* pp){
    auto workspace = configs["workspace"].as<std::string>() + PROFILER_DIR;
    int frequency = configs["profiler"]["collect_frequency"].as<int>();
    return new ProfilingService(workspace, frequency, pp);
}

ProfilerProxy* ConfigurationParser::parse_profiler(YAML::Node configs){
    ProfilerProxy* p;
    YAML::Node hierarchy = configs["data_plane"]["hierarchical_stage"]["hierarchy"];
    YAML::Node frequency = configs["profiler"]["update_frequency"];
    YAML::Node warmup = configs["profiler"]["update_warmup"];

    if(frequency && warmup)
        p = ProfilerProxy::create_instance(hierarchy.size(), frequency.as<int>(), warmup.as<int>());
    else if(frequency)
        p = ProfilerProxy::create_instance(hierarchy.size(), frequency.as<int>());
    else
        p = ProfilerProxy::create_instance(hierarchy.size());

    return p;
}

void ConfigurationParser::create_workspace(YAML::Node configs){
    if(configs["workspace"]){
        int dir_res = mkdir(configs["workspace"].as<std::string>().c_str(), 0777);
        if (dir_res == 0) {
            std::cout << "workspace directory created." << std::endl;
        }
    }else{
        std::cerr << "Please define a workspace directory for your monarch instance.";
        exit(1);
    }
}

//TODO create a yaml_config_check function

Monarch* ConfigurationParser::parse(const std::string &configs_path){
    YAML::Node configs = YAML::LoadFile(configs_path);
    bool profiling = configs["profiler"] && configs["profiler"]["active"].as<bool>();
    bool debug_logs = configs["debug_logs"] && configs["debug_logs"].as<bool>();
    if(profiling || debug_logs){
        create_workspace(configs);
    }
    MonarchBuilder builder = Monarch::create();
    if(profiling){
        auto* profiler = parse_profiler(configs);
        builder.with_profiling_service(parse_profiling_service(configs, profiler));
    }
    if(debug_logs){
        parse_debug_logger(configs, 0);
    }
    YAML::Node data_plane_configs = configs["data_plane"];
    builder.with_hierarchical_stage(parse_hierarchical_stage(data_plane_configs["hierarchical_stage"]))
        .with_control_handler(parse_control_handler(data_plane_configs["handlers"]))
        .with_metadata_container(parse_metadata_container_service(data_plane_configs));
    return builder.build();
}

#if defined(INCLUDE_GRPC)
Stage* ConfigurationParser::parse(RemoteStageBuilder* rbuilder){
    std::string dp_configuration = rbuilder->get_configuration();
    std::istringstream ss(dp_configuration);
    YAML::Node configs = YAML::Load(dp_configuration);
    parse_home_dir(configs);
    auto* hierarchical_stage_builder = parseHierarchicalDataPlaneBuilder(rbuilder, configs);
    auto* stage = parseDataPlane(hierarchical_stage_builder, configs);
    return stage;
}
#endif

