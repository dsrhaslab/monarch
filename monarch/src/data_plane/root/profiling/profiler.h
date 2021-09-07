//
// Created by dantas on 14/03/21.
//

#ifndef THESIS_PROFILER_H
#define THESIS_PROFILER_H

#include <chrono>
#include <tuple>

#include "../../../helpers/logger.h"
#include "stats.h"

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::duration;
using std::chrono::time_point;


struct ReadSubmission {
    time_point<high_resolution_clock> r_start;
    time_point<high_resolution_clock> r_end;
    size_t n;
    bool waited;
    time_point<high_resolution_clock> w_start;
    time_point<high_resolution_clock> w_end;
    ReadSubmission();
};

class Profiler {
    Logger* logger;
    MutexClientStats current_client_stats;
    std::vector<MutexStorageStats*> hierarchy_stats;
    time_point<high_resolution_clock> last_collect_request;

    //related to general operations between read and write;
    //Stats current_operational_stats;
    //std::vector<MutexClientStats*> epoch_snapshots;

    void update_wait_stats(ReadSubmission* submission);
    static void update_stats(MutexStats* stats, long new_value);
    static void update_stats(Stats* stats, long new_value);

protected:
    void submit_read_on_storage(time_point<high_resolution_clock> start, time_point<high_resolution_clock> end, int level);
    void submit_write_on_storage(time_point<high_resolution_clock> start, time_point<high_resolution_clock> end, int level);
    void submit_read_on_storage(int level, size_t n);
    void submit_write_on_storage(int level, size_t n);

public:
    Profiler() = default;
    Profiler(int hierarchy_size, int epochs);
    virtual void configure_service(const std::string& path);
    virtual void submit_client_read(ReadSubmission* submission);
    virtual void submit_ordering(time_point<high_resolution_clock> start, time_point<high_resolution_clock> end);
    virtual void submit_eviction(time_point<high_resolution_clock> start, time_point<high_resolution_clock> end);
    virtual CollectedStats* collect();
};

#endif //THESIS_PROFILER_H