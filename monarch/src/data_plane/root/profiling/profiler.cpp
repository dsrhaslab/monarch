//
// Created by dantas on 14/03/21.
//

#include "profiler.h"

ReadSubmission::ReadSubmission() {
    n = 0;
    waited = false;
}

Profiler::Profiler(int hierarchy_size, int epochs){
    Profiler::logger = new Logger();
    Profiler::last_collect_request = high_resolution_clock::now();

    Profiler::hierarchy_stats.reserve(hierarchy_size);
    for(int i = 0 ; i < hierarchy_size; i++)
        Profiler::hierarchy_stats.push_back(new MutexStorageStats());

    /*
    Profiler::epoch_snapshots.reserve(epochs);
    for(int i = 0; i < epochs; i++)
        Profiler::epoch_snapshots.push_back(new MutexClientStats());
    */
}

void Profiler::configure_service(const std::string& path){
    //TODO fix unique_id;
    logger->configure_service(path, 0);
}

void Profiler::update_stats(MutexStats* stats, long new_value){
    long old_count = stats->content.count++;
    stats->content.mean = (stats->content.mean * old_count + new_value) / (old_count + 1);

    if(new_value > stats->content.max)
        stats->content.max = new_value;
    else if (new_value < stats->content.min)
        stats->content.min = new_value;
}

void Profiler::update_stats(Stats* stats, long new_value){
    long old_count = stats->count++;
    stats->mean = (stats->mean * old_count + new_value) / (old_count + 1);

    if(new_value > stats->max)
        stats->max = new_value;
    else if (new_value < stats->min)
        stats->min = new_value;
}

void Profiler::submit_client_read(ReadSubmission* submission) {
    auto new_d = duration_cast<milliseconds>(submission->r_end - submission->r_start);
    std::unique_lock<std::mutex> ul(current_client_stats);

    current_client_stats.fetched_bytes += submission->n;
    update_stats(&(current_client_stats.read_stats), new_d.count());
    if(submission->waited)
        update_wait_stats(submission);
}

void Profiler::update_wait_stats(ReadSubmission* submission){
    auto new_d = duration_cast<milliseconds>(submission->w_end - submission->w_start);
    update_stats(&(current_client_stats.wait_stats), new_d.count());
}

void Profiler::submit_eviction(time_point<high_resolution_clock> start, time_point<high_resolution_clock> end) {

}

void Profiler::submit_ordering(time_point<high_resolution_clock> start, time_point<high_resolution_clock> end) {

}

void Profiler::submit_read_on_storage(time_point<high_resolution_clock> start, time_point<high_resolution_clock> end, int level) {
    auto new_d = duration_cast<milliseconds>(end - start);
    auto* rc_stats = &(hierarchy_stats[level]->read_stats);
    std::unique_lock<std::mutex> ul(*rc_stats);
    update_stats(rc_stats, new_d.count());
}

void Profiler::submit_write_on_storage(time_point<high_resolution_clock> start, time_point<high_resolution_clock> end, int level) {
    auto new_d = duration_cast<milliseconds>(end - start);
    auto* wc_stats = &(hierarchy_stats[level]->write_stats);
    std::unique_lock<std::mutex> ul(*wc_stats);
    update_stats(wc_stats, new_d.count());
}

void Profiler::submit_read_on_storage(int level, size_t n){
    hierarchy_stats[level]->read_amount += n;
}

void Profiler::submit_write_on_storage(int level, size_t n){
    hierarchy_stats[level]->write_amount += n;
}

CollectedStats* Profiler::collect() {
    std::vector<StorageCollectedStats*> collected_hierarchy_stats;
    collected_hierarchy_stats.reserve(hierarchy_stats.size());
    //growing phase
    current_client_stats.lock();
    auto* collected_client_stats = new ClientStats(&current_client_stats);
    current_client_stats.reset();
    for(auto* mutex_storage_stats : hierarchy_stats){
        mutex_storage_stats->lock();
        collected_hierarchy_stats.push_back(new StorageCollectedStats(mutex_storage_stats));
        mutex_storage_stats->reset();
    }

    //shrinking phase
    current_client_stats.unlock();
    for(auto* mutex_storage_stats : hierarchy_stats)
        mutex_storage_stats->unlock();

    auto start_t = high_resolution_clock::now();
    auto time_diff = duration_cast<milliseconds>(start_t - last_collect_request);
    last_collect_request = start_t;

    for(auto* storage_collected_stats : collected_hierarchy_stats)
        storage_collected_stats->calculate_bandwidth(time_diff.count());

    auto* res = new CollectedStats();
    res->client_stats_snapshot = collected_client_stats;
    res->storage_stats_snapshot = collected_hierarchy_stats;

    return res;
}
