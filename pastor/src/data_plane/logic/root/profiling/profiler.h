//
// Created by dantas on 14/03/21.
//

#ifndef THESIS_PROFILER_H
#define THESIS_PROFILER_H

#include <chrono>
#include <tuple>
#include <iostream>

#include "stats.h"

#define BYTES_CONVERSION_CONSTANT 8 //bits to bytes

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::time_point;


class TimeSubmission {
private:
    time_point<high_resolution_clock> r_start;
    time_point<high_resolution_clock> r_end;

public:
    inline void set_start(){
        r_start = high_resolution_clock::now();
    }

    inline void set_end(){
        r_end = high_resolution_clock::now();
    }

    inline float get_duration(){
        duration<float, std::milli> res = r_end - r_start;
        return res.count();
    }

};

class StatSubmission : public TimeSubmission{
private:
    uint64_t n_bytes;

public:

    StatSubmission(){
        n_bytes = 0;
    }

    inline void set_n_bytes(uint64_t n){
        n_bytes = n;
    }

    inline uint64_t get_n_bytes() const{
        return n_bytes / BYTES_CONVERSION_CONSTANT;
    }
};

class Profiler {
    MutexClientStats current_client_stats;
    MutexPlacementStats current_placement_stats;
    std::vector<MutexStorageStats*> hierarchy_stats;
    time_point<high_resolution_clock> last_collect_request;

    static inline void update_duration(MutexDuration& duration, TimeSubmission& submission){
        std::unique_lock<std::mutex> ul(duration);
        duration.content.n_occurrences++;
        duration.content.n_duration += submission.get_duration();
    }

    static inline void update_stats(MutexStats& stats, StatSubmission& submission){
        std::unique_lock<std::mutex> ul(stats);
        stats.content.n_occurrences++;
        stats.content.n_bytes += submission.get_n_bytes();
        stats.content.n_duration += submission.get_duration();
    }

    static inline void update_stats(Stats& stats, StatSubmission& submission){
        stats.n_occurrences++;
        stats.n_bytes += submission.get_n_bytes();
        stats.n_duration += submission.get_duration();
    }

protected:
    virtual inline void submit_client_read_time(TimeSubmission& submission){
        update_duration(current_client_stats.full_time_stats, submission);
    }

    virtual inline void submit_placement_time(TimeSubmission& submission){
        update_duration(current_placement_stats.full_time_stats, submission);
    }

    virtual inline void submit_storage_client_read(StatSubmission& submission, int level){
        auto& storage_aggregate_stats = hierarchy_stats[level]->aggregate_read_stats;
        std::unique_lock<std::mutex> ul(storage_aggregate_stats);
        update_stats(storage_aggregate_stats.content, submission);
        update_stats(hierarchy_stats[level]->client_read_stats, submission);
        ul.unlock();
        update_stats(current_client_stats.read_stats, submission);
    }

    virtual inline void submit_storage_background_read(StatSubmission& submission, int level){
        auto& storage_aggregate_stats = hierarchy_stats[level]->aggregate_read_stats;
        std::unique_lock<std::mutex> ul(storage_aggregate_stats);
        update_stats(storage_aggregate_stats.content, submission);
        update_stats(hierarchy_stats[level]->background_read_stats, submission);
        ul.unlock();
        update_stats(current_placement_stats.read_stats, submission);
    }

    virtual inline void submit_storage_write(StatSubmission& submission, int level){
        update_stats(hierarchy_stats[level]->write_stats, submission);
        update_stats(current_placement_stats.write_stats, submission);
    }

    virtual inline void submit_client_metadata_request(const std::string& operation, int level){
        hierarchy_stats[level]->metadata_requests[operation]++;
        current_client_stats.metadata_requests[operation]++;
    }

    virtual inline void submit_background_metadata_request(const std::string& operation, int level){
        hierarchy_stats[level]->metadata_requests[operation]++;
        current_placement_stats.metadata_requests[operation]++;
    }

public:
    explicit Profiler(int hierarchy_size){
        last_collect_request = high_resolution_clock::now();
        for(int i = 0 ; i < hierarchy_size; i++)
            hierarchy_stats.push_back(new MutexStorageStats());
    }

    virtual void collect(CollectedStats& collected_stats){
        std::vector<StorageStats*> collected_hierarchy_stats;
        collected_hierarchy_stats.reserve(hierarchy_stats.size());

        //growing phase
        current_client_stats.lock();
        auto* collected_client_stats = new ClientStats(&current_client_stats);
        current_client_stats.reset();
        current_placement_stats.lock();
        auto* collected_placement_stats = new PlacementStats(&current_placement_stats);
        current_placement_stats.reset();

        for(auto& mutex_storage_stats : hierarchy_stats){
            mutex_storage_stats->lock();
            collected_hierarchy_stats.push_back(new StorageStats(mutex_storage_stats));
            mutex_storage_stats->reset();
        }

        //collect start time before lock releases
        auto start_t = high_resolution_clock::now();

        //shrinking phase
        current_client_stats.unlock();
        current_placement_stats.unlock();
        for(auto& mutex_storage_stats : hierarchy_stats)
            mutex_storage_stats->unlock();

        duration<float, std::milli> time_diff = start_t - last_collect_request;
        last_collect_request = start_t;

        collected_stats.client_stats_snapshot = collected_client_stats;
        collected_stats.storage_stats_snapshot = collected_hierarchy_stats;
        collected_stats.background_stats_snapshot = collected_placement_stats;
        collected_stats.time_diff = time_diff.count();
    }
};

#endif //THESIS_PROFILER_H