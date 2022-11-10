//
// Created by dantas on 19/10/20.
//

#ifndef BASIC_CONTROLLER_H
#define BASIC_CONTROLLER_H

#include <thread>
#include <vector>
#include <atomic>
#include <functional>

#if defined BAZEL_BUILD || defined TF_BAZEL_BUILD
#include "third_party/ctpl/ctpl.h"
#else
#include <ctpl.h>
#endif
#include "absl/base/call_once.h"

#include "../../data_governance/metadata/info.h"
#include "../../data_governance/services/metadata_container_service.h"
#include "../../data_governance/payload/file.h"
#include "../../utils/profiling/profiler_proxy.h"
#include "../../utils/logging/logger.h"
#include "../stage.h"
#include "storage_drivers/file_systems/file_system_driver.h"
#include "storage_drivers/memory_buffers/memory_buffer_driver.h"
#include "storage_drivers/file_systems/posix/posix_file_system_driver.h"

class HierarchicalStageBuilder;

class HierarchicalStage : public Stage {
    int n_used_threads;
    bool shared_thread_pool;
    std::vector<ctpl::thread_pool*> storage_hierarchy_thread_pools;
    std::vector<StorageDriver*> storage_hierarchy;
    int source_level;
    int storage_hierarchy_size;

    bool private_debug_enabled;
    Logger* debug_logger;

    explicit HierarchicalStage(int hierarchy_size);
    explicit HierarchicalStage(HierarchicalStage* hs);
    friend class HierarchicalStageBuilder;

    void debug_write(const std::string& msg);
    bool debug_is_activated();

public:
    static HierarchicalStageBuilder* create(int hierarchy_size);

    int get_source_level();

    bool has_staging_levels();

    bool type_is(StorageDriverType type, int level);

    bool subtype_is(StorageDriverSubType subtype, int level);

    bool driver_state_type_is(StorageDriverStateType, int level);

    StorageDriver* get_driver(int level);

    FileSystemDriver* get_source_driver();

    FileSystemDriver* get_file_system_driver(int level);

    PosixFileSystemDriver* get_posix_file_system_driver(int level);

    MemoryBufferDriver* get_memory_buffer_driver(int level);

    void make_blocking_driver_wrappers();

    void make_eventual_driver_wrappers();

    int find_free_level(Info* i, int offset);

    int find_free_level(Info* i);

    StorageDriver* find_free_storage_driver(Info* i, int offset);

    StorageDriver* find_free_storage_driver(Info* i);

    int alloc_free_level(Info* i, int offset);

    int eventual_free_level(Info* i, int offset);

    void apply_to_fs_drivers(std::function<void(FileSystemDriver*)> func);

    size_t get_capacity(int level);

    size_t get_full_capacity();

    ctpl::thread_pool* t_pool(int storage_index);

    std::vector<std::string> configs();

    void init(std::vector<std::string>& dirs);

    void print();
};

#endif // BASIC_CONTROLLER_H
