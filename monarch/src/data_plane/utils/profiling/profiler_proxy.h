//
// Created by dantas on 17/03/21.
//

#ifndef MONARCH_PROFILER_PROXY_H
#define MONARCH_PROFILER_PROXY_H

#define DEFAULT_UPDATE_FREQUENCY 0;
#define DEFAULT_INITIAL_WARMUP 0;

#include "profiler.h"

struct StorageStatsUpdateCounters {
    std::atomic<int> background_read_counter;
    std::atomic<int> client_read_counter;
    std::atomic<int> metadata_counter;
    std::atomic<int> write_counter;
};

//This is a singleton class
class ProfilerProxy : public Profiler {
private:
    static ProfilerProxy* instance_;
    static std::mutex mutex_;
    bool init_done_;

    inline bool submitable(std::atomic<int>* counter, bool incr){
        int current;
        if(incr){
            current = (*counter)++;
        }else{
            current = *counter;
        }
        return current % updates_frequency_ == 0 && current > initial_warmup_;
    }

    friend class ProfilingService;

protected:
    int hierarchy_size_;
    int updates_frequency_ = DEFAULT_UPDATE_FREQUENCY;
    int initial_warmup_ = DEFAULT_INITIAL_WARMUP;
    std::vector<StorageStatsUpdateCounters*> storage_counters;

    ProfilerProxy() {
        init_done_ = false;
    }

    void init_profiler_proxy(int hierarchy_size){
        init_done_ = true;
        hierarchy_size_ = hierarchy_size;
        storage_counters.reserve(hierarchy_size);
        for(int i = 0; i < hierarchy_size; i++)
            storage_counters.push_back(new StorageStatsUpdateCounters());
    }

    void init_profiler_proxy(int hierarchy_size, int update_frequency){
        init_profiler_proxy(hierarchy_size);
        updates_frequency_ = update_frequency;
    }

    void init_profiler_proxy(int hierarchy_size, int update_frequency, int initial_warmup){
        init_profiler_proxy(hierarchy_size, update_frequency);
        initial_warmup_ = initial_warmup;
    }

    void init(int hierarchy_size){
        Profiler::init_variables(hierarchy_size);
        init_profiler_proxy(hierarchy_size);
    };

    void init(int hierarchy_size, int update_frequency){
        Profiler::init_variables(hierarchy_size);
        init_profiler_proxy(hierarchy_size, update_frequency);
    };

    void init(int hierarchy_size, int update_frequency, int initial_warmup){
        Profiler::init_variables(hierarchy_size);
        init_profiler_proxy(hierarchy_size, update_frequency, initial_warmup);
    };

    explicit ProfilerProxy(int hierarchy_size) : Profiler(hierarchy_size) {
        init_profiler_proxy(hierarchy_size);
    }

    ProfilerProxy(int hierarchy_size, int update_frequency) : ProfilerProxy(hierarchy_size) {
        init_profiler_proxy(hierarchy_size, update_frequency);
    }

    ProfilerProxy(int hierarchy_size, int update_frequency, int initial_warmup) : ProfilerProxy(hierarchy_size, update_frequency) {
        init_profiler_proxy(hierarchy_size, update_frequency, initial_warmup);
    }

public:

    ProfilerProxy(ProfilerProxy& other) = delete;
    void operator=(const ProfilerProxy &) = delete;

    inline bool is_activated(){
        return init_done_;
    }

    inline void submit_client_read_time(TimeSubmission& submission) override{
        Profiler::submit_client_read_time(submission);
    }

    inline void submit_placement_time(TimeSubmission& submission) override{
        Profiler::submit_placement_time(submission);
    }

    inline void submit_storage_client_read(StatSubmission& submission, int level) override{
        if(updates_frequency_ == 0 || submitable(&storage_counters[level]->client_read_counter, true))
            Profiler::submit_storage_client_read(submission, level);
    }

    inline void submit_storage_background_read(StatSubmission& submission, int level) override {
        if(updates_frequency_ == 0 || submitable(&storage_counters[level]->background_read_counter, true))
            Profiler::submit_storage_background_read(submission, level);
    }

    inline void submit_storage_write(StatSubmission& submission, int level) override{
        if(updates_frequency_ == 0 || submitable(&storage_counters[level]->write_counter, true))
            Profiler::submit_storage_write(submission, level);
    }

    inline void submit_client_metadata_request(const std::string& operation, int level) override{
        if(updates_frequency_ == 0 || submitable(&storage_counters[level]->metadata_counter, true))
            Profiler::submit_client_metadata_request(operation, level);
    }

    inline void submit_background_metadata_request(const std::string& operation, int level) override{
        if(updates_frequency_ == 0 || submitable(&storage_counters[level]->metadata_counter, true))
            Profiler::submit_background_metadata_request(operation, level);
    }

    void collect(CollectedStats& collected_stats) override{
        Profiler::collect(collected_stats);
    };

    static ProfilerProxy* create_instance(int hierarchy_size);

    static ProfilerProxy* create_instance(int hierarchy_size, int update_frequency);

    static ProfilerProxy* create_instance(int hierarchy_size, int update_frequency, int initial_warmhup);

    static ProfilerProxy* get_instance();
};

#endif //MONARCH_PROFILER_PROXY_H
