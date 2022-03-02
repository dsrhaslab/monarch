//
// Created by dantas on 17/03/21.
//

#ifndef THESIS_PROFILER_PROXY_H
#define THESIS_PROFILER_PROXY_H

#define DEFAULT_UPDATE_FREQUENCY 0;
#define DEFAULT_INITIAL_WARMUP 0;

#include "profiler.h"

struct StorageStatsUpdateCounters {
    std::atomic<int> background_read_counter;
    std::atomic<int> client_read_counter;
    std::atomic<int> metadata_counter;
    std::atomic<int> write_counter;
};

class ProfilerProxy : public Profiler {
private:
    int updates_frequency;
    int initial_warmup;
    std::vector<StorageStatsUpdateCounters*> storage_counters;

    inline bool submitable(std::atomic<int>* counter, bool incr){
        int current;
        if(incr){
            current = (*counter)++;
        }else{
            current = *counter;
        }
        return current % updates_frequency == 0 && current > initial_warmup;
    }

public:
    int hierarchy_size_;

public:
    explicit ProfilerProxy(int hierarchy_size) : Profiler(hierarchy_size) {
        updates_frequency = DEFAULT_UPDATE_FREQUENCY;
        initial_warmup = DEFAULT_INITIAL_WARMUP;
        hierarchy_size_ = hierarchy_size;
        storage_counters.reserve(hierarchy_size);
        for(int i = 0; i < hierarchy_size; i++)
            storage_counters.push_back(new StorageStatsUpdateCounters());
    }

    ProfilerProxy(int hierarchy_size, int update_frequency) : ProfilerProxy(hierarchy_size) {
        updates_frequency = update_frequency;
    }

    ProfilerProxy(int hierarchy_size, int update_frequency, int initial_warmhup_) : ProfilerProxy(hierarchy_size, update_frequency) {
        initial_warmup = initial_warmhup_;
    }

    //TODO submitable logic
    inline void submit_client_read_time(TimeSubmission& submission) override{
        Profiler::submit_client_read_time(submission);
    }

    inline void submit_placement_time(TimeSubmission& submission) override{
        Profiler::submit_placement_time(submission);
    }

    inline void submit_storage_client_read(StatSubmission& submission, int level) override{
        if(updates_frequency == 0 || submitable(&storage_counters[level]->client_read_counter, true))
            Profiler::submit_storage_client_read(submission, level);
    }

    inline void submit_storage_background_read(StatSubmission& submission, int level) override {
        if(updates_frequency == 0 || submitable(&storage_counters[level]->background_read_counter, true))
            Profiler::submit_storage_background_read(submission, level);
    }

    inline void submit_storage_write(StatSubmission& submission, int level) override{
        if(updates_frequency == 0 || submitable(&storage_counters[level]->write_counter, true))
            Profiler::submit_storage_write(submission, level);
    }

    inline void submit_client_metadata_request(const std::string& operation, int level) override{
        if(updates_frequency == 0 || submitable(&storage_counters[level]->metadata_counter, true))
            Profiler::submit_client_metadata_request(operation, level);
    }

    inline void submit_background_metadata_request(const std::string& operation, int level) override{
        if(updates_frequency == 0 || submitable(&storage_counters[level]->metadata_counter, true))
            Profiler::submit_background_metadata_request(operation, level);
    }

    void collect(CollectedStats& collected_stats) override{
        Profiler::collect(collected_stats);
    };

};


#endif //THESIS_PROFILER_PROXY_H
