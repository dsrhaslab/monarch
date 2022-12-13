//
// Created by dantas on 15/08/22.
//

#ifndef MONARCH_H
#define MONARCH_H

#include "absl/strings/strip.h"

#include "stage.h"
#include "../utils/profiling/profiling_service.h"
#include "../handlers/control_handler.h"
#include "../data_governance/services/metadata_container_service.h"
#include "hierarchical/hierarchical_stage.h"

class MonarchBuilder;

struct RequestArguments{
    RequestArguments(int level, bool client_req, bool _64_option)
        : level_(level), client_req_(client_req), _64_option_(_64_option) {}
    int level_;
    bool client_req_;
    bool _64_option_;
};

struct CloseRequestArguments{
    CloseRequestArguments(int level, bool client_req)
            : level_(level), client_req_(client_req) {}
    int level_;
    bool client_req_;
};

class Monarch : public Stage {
private:
    ControlHandler* control_handler;
    HierarchicalStage* hierarchical_stage;
    MetadataContainerService* metadata_container;
    //TODO : This could be an isolated "monitor"
    off_t epoch_size_signal;
    std::mutex epoch_change_mutex;
    bool becomes_full;
    bool private_debug_enabled;
    Logger* debug_logger;
    ProfilerProxy* profiler;
    ProfilingService* profiling_service;

    explicit Monarch();

    absl::string_view decode_filename(absl::string_view full_path);

    inline Status<Info*> get_metadata(const char *pathname);

    int open_from_storage(const char *pathname, int flags, mode_t mode, const RequestArguments& req_args);

    int open_from_storage(const char *pathname, int flags, const RequestArguments& req_args);

    void print_open(const char *pathname, const RequestArguments& req_args);

    void check_epoch_finish(size_t n);

    size_t get_request_size(Info* i, size_t n, off_t offset, int storage_level);

    void print_debug_client_read(int storage_level, size_t requested_size, off_t offset, const std::string& name);

    void print_debug_offset_overflow(Info* i, off_t offset, const std::string& operation);

    ssize_t base_read(Info* i, char* result, size_t n, off_t offset, const RequestArguments& req_args);

    Status<ssize_t> base_read_from_storage(Info* i, char* result, size_t n, off_t offset, const RequestArguments& req_args);

    void* base_mmap(void* addr, size_t length, int prot, int flags, Info* i, off_t offset, int storage_level);

    Status<void*> mmap_from_storage(void *addr, size_t length, int prot, int flags, Info* i, off_t offset, int level);

    Status<void*> base_mmap_from_storage(void *addr, size_t length, int prot, int flags, Info* i, off_t offset, int level);

    Status<ssize_t> base_write(File* f, int level);

    int close_from_storage(int fd, int target_level, const CloseRequestArguments& req_args);

    friend class MonarchBuilder;

    friend class PlacementHandler;

protected:

    Status<ssize_t> write(File* f, int level);

    int base_open(Info*, int flags, mode_t mode, const RequestArguments& req_args);

    int base_open(Info*, int flags, const RequestArguments& req_args);

    Status<ssize_t> read_from_storage(Info* i, char* result, size_t n, off_t offset, const RequestArguments& req_args);

    int base_close(Info* i, const CloseRequestArguments& req_args);

    HierarchicalStage* get_hierarchical_stage();

    MetadataContainerService* get_metadata_container_service();

    File* remove_for_copy(Info* i, int level);

    Status<ssize_t> remove(Info* i, int level);

public:
    static MonarchBuilder create();

    Status<Info*> open(const char *pathname, int flags, mode_t mode, bool _64_option);

    Status<Info*> open(const char *pathname, int flags, bool _64_option);

    ssize_t read(Info* i, char* result, size_t n, off_t offset, bool _64_option);

    void* mmap(void* addr, size_t length, int prot, int flags, Info* i, off_t offset);

    int close(Info* i);

    void init();

    void start_added_services();

    void print() override;

    void debug_write(const std::string& msg);

    bool debug_is_activated();

    //Destructor must be called for the profiling_service to work properly
    ~Monarch();
};

#endif // MONARCH_H