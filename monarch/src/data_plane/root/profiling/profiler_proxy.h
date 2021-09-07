//
// Created by dantas on 17/03/21.
//

#ifndef THESIS_PROFILER_PROXY_H
#define THESIS_PROFILER_PROXY_H

#define DEFAULT_UPDATE_FREQUENCY 1000;
#define DEFAULT_INITIAL_WARMUP 0;

#include "profiler.h"

struct StorageStatsUpdateCounters {
    std::atomic<int> read_counter;
    std::atomic<int> write_counter;
    std::atomic<int> samples_life_counter;
    std::atomic<int> occupation_counter;
};

class ProfilerProxy : public Profiler {
    bool service_activated;
    int updates_frequency;
    int initial_warmup;
    std::atomic<int> client_stats_update_counter;
    std::vector<StorageStatsUpdateCounters*> storage_counters;

    bool submitable(std::atomic<int>* counter);

public:
    ProfilerProxy();
    ProfilerProxy(int hierarchy_size, int epochs);
    ProfilerProxy(int hierarchy_size, int epochs, int update_frequency);
    ProfilerProxy(int hierarchy_size, int epochs, int update_frequency, int initial_warmhup);
    void configure_service(const std::string& path);
    void submit_client_read(ReadSubmission* submission) override;
    void submit_read_on_storage(time_point<high_resolution_clock> start, time_point<high_resolution_clock> end, int level, size_t n);
    void submit_write_on_storage(time_point<high_resolution_clock> start, time_point<high_resolution_clock> end, int level, size_t n);
    void submit_ordering(time_point<high_resolution_clock> start, time_point<high_resolution_clock> end) override;
    void submit_eviction(time_point<high_resolution_clock> start, time_point<high_resolution_clock> end) override;
    CollectedStats* collect() override;
    bool is_activated();
};


#endif //THESIS_PROFILER_PROXY_H
