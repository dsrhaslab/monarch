//
// Created by dantas on 25/10/20.
//

#include <iostream>
#include <cstdio>
#include <sys/stat.h>

#include "configuration_parser.h"
#include "../root/storage_drivers/data_storage_driver_builder.h"
#include "../handlers/control_handler_builder.h"

#if defined(INCLUDE_GRPC)
#include "../remote/remote_stage_builder.h"
#endif
#define DEBUG_DIR "/debugger"
#define PROFILER_DIR "/profiling"

DataStorageDriver* ConfigurationParser::parseStorageDriver(YAML::Node driver, YAML::Node configs, int level){
    //default is disk
    DataStorageDriverBuilder storage_builder = DataStorageDriver::create(FILE_SYSTEM);

    auto storage_type = driver["type"].as<std::string>();

    if (storage_type == "tbb_memory_buffer")
        storage_builder = DataStorageDriver::create(TBB_MEMORY_BUFFER);
    else if (storage_type == "ph_memory_buffer")
        storage_builder = DataStorageDriver::create(PH_MEMORY_BUFFER);
    else if (storage_type == "file_system"){
        if(driver["block_size"]){
            if(driver["block_size"].as<std::string>() == "max"){
                storage_builder.with_block_size(INT32_MAX);
            }
            else{
                storage_builder.with_block_size(driver["block_size"].as<int>());
            }
        }
        storage_builder.with_storage_prefix(driver["prefix"].as<std::string>());
    }
    else {
        std::cerr << "storage driver type not supported\n";
        exit(1);
    }
    YAML::Node mss = driver["max_storage_size"];
    YAML::Node msot = driver["max_storage_occupation_threshold"];

    if(mss && msot){
        storage_builder.with_allocation_capabilities(mss.as<size_t>(), msot.as<float>(), false);
    }else if (mss)
        storage_builder.with_allocation_capabilities(mss.as<size_t>(), false);

    storage_builder.with_hierarchy_level(level);

    return storage_builder.build();
}


ProfilerProxy* ConfigurationParser::parseProfiler(YAML::Node root_configs){
    YAML::Node hierarchy = root_configs["data_plane"]["hierarchy"];
    ProfilerProxy* p;

    YAML::Node frequency = root_configs["profiler"]["update_frequency"];
    YAML::Node warmup = root_configs["profiler"]["update_warmup"];

    if(frequency && warmup)
        p = new ProfilerProxy(hierarchy.size(), frequency.as<int>(), warmup.as<int>());
    else if(frequency)
        p = new ProfilerProxy(hierarchy.size(), frequency.as<int>());
    else
        p = new ProfilerProxy(hierarchy.size());

    return p;
}

ProfilingService* ConfigurationParser::parseProfilingService(YAML::Node root_configs, ProfilerProxy* pp){
    auto home = root_configs["home_dir"].as<std::string>() + PROFILER_DIR;
    int frequency = root_configs["profiler"]["collect_frequency"].as<int>();
    return new ProfilingService(home, frequency, pp);
}


void ConfigurationParser::parse_rate_limiter(HierarchicalDataPlaneBuilder* builder, YAML::Node root_configs){
    YAML::Node configs = root_configs["data_plane"];
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

void ConfigurationParser::parse_thread_pool(HierarchicalDataPlaneBuilder* builder, YAML::Node root_configs){
    YAML::Node configs = root_configs["data_plane"];
    YAML::Node hierarchy = configs["hierarchy"];
    bool shared_tpool = false;
    auto tpool_size = configs["shared_tpool_size"];

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

void ConfigurationParser::parse_storage_matrix(HierarchicalDataPlaneBuilder* builder, YAML::Node root_configs, int world_size, int num_workers){
    YAML::Node configs = root_configs["data_plane"];
    YAML::Node hierarchy = configs["hierarchy"];
    std::vector<std::vector<DataStorageDriver*>> matrix;
    for (int j = 0; j < world_size * num_workers; j++) {
        std::vector<DataStorageDriver*> drivers;
        matrix.push_back(drivers);
        for(int i = 0; i < hierarchy.size(); i++) {
            auto driver = hierarchy[i];
            matrix[j].push_back(parseStorageDriver(driver, configs, i));
        }
    }
    builder->with_storage_hierarchy(matrix);
}

#if defined(INCLUDE_GRPC)
HierarchicalDataPlaneBuilder* ConfigurationParser::parseHierarchicalDataPlaneBuilder(RemoteStageBuilder* rbuilder, YAML::Node root_configs){
    YAML::Node configs = root_configs["data_plane"];
    YAML::Node hierarchy = configs["hierarchy"];
    HierarchicalDataPlaneBuilder* builder = HierarchicalDataPlane::create(rbuilder->get_instance_identifier(),
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

//TODO Check this. It ignores rank, world_size....etc
HierarchicalDataPlaneBuilder* ConfigurationParser::parseHierarchicalDataPlaneBuilder(YAML::Node root_configs){
    YAML::Node configs = root_configs["data_plane"];
    YAML::Node hierarchy = configs["hierarchy"];
    HierarchicalDataPlaneBuilder* builder = HierarchicalDataPlane::create(0,1,1, hierarchy.size());
    parse_storage_matrix(builder, root_configs, 1, 1);
    parse_thread_pool(builder, root_configs);
    parse_rate_limiter(builder, root_configs);

    if(configs["storage_sync_timeout"]){
        builder->with_storage_synchronization_timeout(configs["storage_sync_timeout"].as<int>());
    }

    if(root_configs["debug"].as<std::string>() == "true")
        builder->with_debug_enabled(root_configs["home_dir"].as<std::string>() + DEBUG_DIR, 0);

    auto type = configs["type"].as<std::string>();

    builder->with_metadata_container(parseMetadataContainerService(root_configs));
    builder->with_type(type);

    if(root_configs["profiler"] && root_configs["profiler"]["active"].as<std::string>() == "true") {
        auto* profiler = parseProfiler(root_configs);
        builder->with_profiling_enabled(profiler, parseProfilingService(root_configs, profiler));
    }

    return builder;
}

MetadataContainerService<FileInfo>* ConfigurationParser::parseMetadataContainerService(YAML::Node root_configs){
    YAML::Node metadata_container_configs = root_configs["metadata_container"];

    YAML::Node hierarchy = root_configs["data_plane"]["hierarchy"];
    int hierarchy_size = hierarchy.size();
    auto source_dir = hierarchy[hierarchy_size - 1]["prefix"].as<std::string>();

    auto e = metadata_container_configs["epochs"];

    int epochs = 0;
    if (e){
        epochs = e.as<int>();
    }

    YAML::Node configs = root_configs["data_plane"];
    auto type = configs["type"].as<std::string>();
    auto has_shareable_file_descriptors = false;
    if(configs["has_shareable_file_descriptors"] && configs["has_shareable_file_descriptors"].as<std::string>() == "true") {
        has_shareable_file_descriptors = true;
    }

    auto* mcs = new MetadataContainerService<FileInfo>(source_dir, type);
    mcs->set_epochs(epochs);
    mcs->set_world_size(1);
    mcs->set_distributed_id(0);
    mcs->set_storage_source_level(hierarchy_size - 1);
    mcs->set_shareable_file_descriptors(has_shareable_file_descriptors);

    if(metadata_container_configs["regex"]){
        std::string regex = metadata_container_configs["regex"].as<std::string>();
        mcs->set_train_files_regex(regex);
    }

    return mcs;
}

ControlPolicy ConfigurationParser::parse_control_policy(YAML::Node type_configs){
    if(type_configs["control_policy"]){
        std::string pol = type_configs["control_policy"].as<std::string>();
        if(pol == "lock_ordered")
            return LOCK_ORDERED;
        else if(pol == "queue_ordered")
            return QUEUE_ORDERED;
        else if(pol== "single_thread_ordered")
            return SINGLE_THREAD_ORDERED;
        else if(pol == "solo_placement")
            return SOLO_PLACEMENT;
        else {
            std::cerr << "Bad control policy\n";
            exit(1);
        }
    }else{
        return LOCK_ORDERED;
    }
}

PlacementPolicy ConfigurationParser::parse_placement_policy(YAML::Node type_configs){
    if(type_configs["placement_policy"]){
        std::string pol = type_configs["placement_policy"].as<std::string>();
        if(pol == "push_down")
            return PUSH_DOWN;
        else if(pol == "first_level_only")
            return FIRST_LEVEL_ONLY;
        else{
            std::cerr << "Bad placement policy\n";
            exit(1);
        }
    }else{
        return PUSH_DOWN;
    }
}

ReadPolicy ConfigurationParser::parse_read_policy(YAML::Node type_configs){
    if(type_configs["read_policy"]){
        std::string strat = type_configs["read_policy"].as<std::string>();
        if(strat == "wait_enforced")
            return WAIT_ENFORCED;
        else if(strat == "relaxed")
            return RELAXED;
        else {
            std::cerr << "Bad read policy\n";
            exit(1);
        }
    }else{
        return WAIT_ENFORCED;
    }
}

EvictCallType ConfigurationParser::parse_evict_call_type(YAML::Node type_configs){
    if(type_configs["evict_call_type"]){
        std::string strat = type_configs["evict_call_type"].as<std::string>();
        if(strat == "client")
            return CLIENT;
        else if(strat == "housekeeper")
            return HOUSEKEEPER;
        else {
            std::cerr << "Bad evict call type\n";
            exit(1);
        }
    }else{
        return CLIENT;
    }
}


ControlHandlerBuilder* ConfigurationParser::parseControlHandler(YAML::Node configs){
    auto* chb = new ControlHandlerBuilder();
    YAML::Node t_configs = configs["data_plane"]["type_configs"];

    chb->with_placement_policy(parse_placement_policy(t_configs));
    chb->with_evict_call_type(parse_evict_call_type(t_configs));
    chb->with_control_policy(parse_control_policy(t_configs));

    if(t_configs["async_placement"]){
         auto str = t_configs["async_placement"].as<std::string>();
         if(str == "true"){
             chb->with_async_placement(true);
         }
         else{
             chb->with_async_placement(false);
         }
    }

    if(t_configs["dedicated_thread_pool"]){
        auto str = t_configs["dedicated_thread_pool"].as<std::string>();
        if(str == "true"){
            chb->with_dedicated_thread_pool(true);
        }
        else{
            chb->with_dedicated_thread_pool(false);
        }
    }

    return chb;
}

PrefetchDataPlaneBuilder* ConfigurationParser::parsePrefetchDataPlaneBuilder(HierarchicalDataPlane *root_data_plane, YAML::Node configs){
    PrefetchDataPlaneBuilder* builder = PrefetchDataPlane::create(root_data_plane);

    YAML::Node t_configs = configs["type_configs"];

    if(t_configs["eviction_percentage"])
        builder->with_eviction_percentage(t_configs["eviction_percentage"].as<float>());

    if(t_configs["prefetch_tpool_size"])
        builder->with_prefetch_thread_pool_size(t_configs["prefetch_tpool_size"].as<int>());

    builder->with_read_policy(parse_read_policy(t_configs));

    return builder;
}


DataPlane* ConfigurationParser::parseDataPlane(HierarchicalDataPlaneBuilder* hierarchical_data_plane_builder, YAML::Node root_configs){
    YAML::Node configs = root_configs["data_plane"];
    auto data_plane_type = configs["type"].as<std::string>();
    if (data_plane_type == "prefetch_enabled")
        return parsePrefetchDataPlaneBuilder(hierarchical_data_plane_builder->build(), configs)->build();
    else if (data_plane_type == "root_standalone")
        return hierarchical_data_plane_builder->build();
    else {
        std::cerr << "data plane type not supported\n";
        return nullptr;
    }
}


#if defined(INCLUDE_GRPC)
DataPlane* ConfigurationParser::parse(RemoteStageBuilder* rbuilder){
    std::string dp_configuration = rbuilder->get_configuration();
    std::istringstream ss(dp_configuration);
    YAML::Node configs = YAML::Load(dp_configuration);
    parse_home_dir(configs);
    auto* hierarchical_data_plane_builder = parseHierarchicalDataPlaneBuilder(rbuilder, configs);
    auto* data_plane = parseDataPlane(hierarchical_data_plane_builder, configs);
    return data_plane;
}
#endif

DataPlane* ConfigurationParser::parse(const std::string &configs_path){
    YAML::Node configs = YAML::LoadFile(configs_path);
    parse_home_dir(configs);
    auto* hierarchical_data_plane_builder = parseHierarchicalDataPlaneBuilder(configs);
    hierarchical_data_plane_builder->with_control_handler(parseControlHandler(configs));
    auto* data_plane = parseDataPlane(hierarchical_data_plane_builder, configs);
    return data_plane;
}


void ConfigurationParser::parse_home_dir(YAML::Node configs){
    if(configs["home_dir"]){
        int dir_res = mkdir(configs["home_dir"].as<std::string>().c_str(), 0777);
        if (dir_res == 0) {
            std::cout << "home directory created." << std::endl;
        }
    }else{
        std::cerr << "Please define a home directory for your data_plane instance.";
        exit(1);
    }
}
