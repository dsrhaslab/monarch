//
// Created by dantas on 19/10/20.
//

#ifndef BASIC_CONTROLLER_H
#define BASIC_CONTROLLER_H

#include <thread>
#include <vector>
#include <atomic>
#include "rate_limiting/rate_limiter.h"
#include "rate_limiting/client_watch_rate_limiter.h"
#include "storage_drivers/data_storage_driver.h"
#include "storage_drivers/file_system_driver.h"
#include "profiling/profiler_proxy.h"
#include "profiling/profiling_service.h"
#include "../data_plane.h"
#include "../metadata/placed_file_info.h"
#include "../metadata/metadata_container_service.h"
#include "../metadata/file.h"
#include "../../../helpers/logger.h"
#if defined BAZEL_BUILD || defined TF_BAZEL_BUILD
#include "third_party/ctpl/ctpl.h"
#else
#include <ctpl.h>
#endif
#include "absl/base/call_once.h"

class HierarchicalDataPlaneBuilder;
class ControlHandler;

class HierarchicalDataPlane : public DataPlane{
    int neighbours_index;
    int storage_sync_timeout;
    int matrix_index;
    std::string type;
    bool shared_thread_pool;
    int total_used_threads;
    std::vector<ctpl::thread_pool*> storage_hierarchy_thread_pools;
    std::vector<std::vector<DataStorageDriver*>> storage_hierarchical_matrix;
    ClientWatchRateLimiter* rate_limiter;

    //for transparent
    absl::once_flag once_;

    ProfilingService* profiling_service;
    uint64_t epoch_size_signal;
    std::mutex epoch_change_mutex;

protected:
    // TODO for prefetching. If application_id not defined use instance_id.
    int instance_id;
    int rank;
    int world_size;
    int worker_id;
    int num_workers;
    bool transparent_api;
    MetadataContainerService<FileInfo>* metadata_container;
    ctpl::thread_pool* synchronization_thread_pool;
    ControlHandler* control_handler;

public:
    Logger* debug_logger;
    ProfilerProxy* profiler;
    std::atomic<int> placed_samples;
    int storage_hierarchy_size;
    bool reached_stability;
    bool becomes_full;
    //If the application reads chunks from the same glob in parallel or sequentially.
    bool uses_large_seq_reads;

private:
    explicit HierarchicalDataPlane(int instance_id, int world_size, int number_of_workers, int hierarchy_size);
    int get_storage_hierarchical_matrix_index(int rank, int worker_id);
    Status<ssize_t> base_read_from_storage(FileInfo* fi, char* result, uint64_t offset, size_t n, int level, bool _64_option);
    ssize_t base_read(FileInfo* fi, char* result, uint64_t offset, size_t n, bool _64_option);
    void *mmap(void *addr, size_t length, int prot, int flags, FileInfo* fi, off_t offset);
    Status<void*> base_mmap_from_storage(void *addr, size_t length, int prot, int flags, FileInfo* fi, off_t offset, int level);
    Status<void*> mmap_from_storage(void *addr, size_t length, int prot, int flags, FileInfo* fi, off_t offset, int level);
    void *base_mmap(void *addr, size_t length, int prot, int flags, FileInfo* fi, off_t offset);
    void metadata_init();
    void check_open_phase(FileInfo* fi, int storage_level);
    void check_close_phase(FileInfo* fi, size_t requested_size, uint64_t offset, int storage_level);
    void check_epoch_finish(size_t n);

protected:
    explicit HierarchicalDataPlane(HierarchicalDataPlane* hdp);
    int free_level(int rank, int worker_id, FileInfo* fi, int offset);
    bool is_allocable(int rank, int worker_id, int level);
    AllocableDataStorageDriver* get_alloc_driver(int rank, int worker_id, int level);
    DataStorageDriver* get_driver(int rank, int worker_id, int level);
    DataStorageDriver* get_driver(int level);
    bool enforce_rate_limit();
    void await_termination();
    void set_total_jobs(int iter_size);
    ssize_t read(FileInfo* fi, char* result, uint64_t offset, size_t n, bool _64_option);
    int get_list_index(int rank, int worker_id, int index);
    void synchronize_storages(int offset);
    void start_sync_loop(int offset);

public:
    ~HierarchicalDataPlane();
    friend class HierarchicalDataPlaneBuilder;
    static HierarchicalDataPlaneBuilder* create(int instance_id_, int world_size_, int number_of_workers, int hierarchy_size);
    absl::string_view decode_filename(absl::string_view full_path) override;
    int open(const char *pathname, int flags, mode_t mode, bool _64_option) override;
    int open(const char *pathname, int flags, bool _64_option) override;
    void *mmap (void *addr, size_t length, int prot, int flags, int fd, off_t offset) override;
    ssize_t pread(int fildes, char *result, size_t nbyte, uint64_t offset, bool _64_option) override;
    int close(int fildes) override;
    ssize_t read(const std::string& filename, char* result, uint64_t offset, size_t n) override;
    size_t get_file_size(const std::string &filename) override;
    size_t get_file_size_from_id(int id) override;
    int get_target_class(const std::string &filename) override;
    ssize_t read_from_id(int file_id, char* result, uint64_t offset, size_t n) override;
    ssize_t read_from_id(int file_id, char* result) override;
    int get_target_class_from_id(int id) override;
    virtual std::vector<std::string> configs();
    void print() override;
    void init(bool transparent_api_) override;
    void start() override;
    int get_file_count() override;
    void set_distributed_params(int rank, int worker_id) override;

    void enforce_rate_brake(int new_brake_id);

    void apply_job_termination();
    void apply_job_start();

    bool is_file_system(int level);
    FileSystemDriver* get_fs_driver(int level);
    AllocableDataStorageDriver* get_alloc_driver(int level);
    BlockingAllocableDataStorageDriver* get_blocking_alloc_driver(int level);
    EventualAllocableDataStorageDriver* get_eventual_alloc_driver(int level);

    bool is_allocable(int level);
    bool is_blocking(int level);

    ctpl::thread_pool* t_pool(int storage_index);

    int free_level(FileInfo* fi, int offset);
    int eventual_free_level(FileInfo* fi, int offset);
    bool is_eventual(int level);

    int alloc_free_level(FileInfo* fi, int offset);

    Status<ssize_t> read_from_storage(bool client_req, File* f, int level);
    Status<ssize_t> read_from_storage(bool client_req, FileInfo* fi, char* result, uint64_t offset, size_t n, int level, bool _64_option);

    Status<ssize_t> remove(FileInfo* fi, int level);

    File* remove_for_copy(FileInfo* fi, int level);

    Status<ssize_t> write(File* f, int level);

    void enforce_rate_continuation(int new_brake_release_id);

    void make_blocking_drivers();
    void make_eventual_drivers();


    virtual void debug_write(const std::string& msg);
};

#endif // BASIC_CONTROLLER_H
