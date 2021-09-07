//
// Created by dantas on 17/03/21.
//

#include "profiler_proxy.h"

ProfilerProxy::ProfilerProxy(){
    service_activated = false;
}

ProfilerProxy::ProfilerProxy(int hierarchy_size, int epochs) : Profiler(hierarchy_size, epochs) {
    updates_frequency = DEFAULT_UPDATE_FREQUENCY;
    initial_warmup = DEFAULT_INITIAL_WARMUP;
    service_activated = false;
    storage_counters.reserve(hierarchy_size);
    for(int i = 0; i < hierarchy_size; i++)
        storage_counters.push_back(new StorageStatsUpdateCounters());
}

ProfilerProxy::ProfilerProxy(int hierarchy_size, int epochs, int uf) : ProfilerProxy(hierarchy_size, epochs) {
    updates_frequency = uf;
}

ProfilerProxy::ProfilerProxy(int hierarchy_size, int epochs, int uf, int iw) : ProfilerProxy(hierarchy_size, epochs, uf){
    updates_frequency = uf;
    initial_warmup = iw;
}

void ProfilerProxy::configure_service(const std::string& path){
    service_activated = true;
    Profiler::configure_service(path);
}

bool ProfilerProxy::submitable(std::atomic<int>* counter){
    int current = (*counter)++;
    return current % updates_frequency == 0 && current > initial_warmup;
}

void ProfilerProxy::submit_client_read(ReadSubmission* submission){
    if(submitable(&client_stats_update_counter))
        Profiler::submit_client_read(submission);
}

void ProfilerProxy::submit_read_on_storage(time_point<high_resolution_clock> start, time_point<high_resolution_clock> end, int level, size_t n){
    Profiler::submit_read_on_storage(level, n);
    if (submitable(&storage_counters[level]->read_counter))
        Profiler::submit_read_on_storage(start, end, level);
}

void ProfilerProxy::submit_write_on_storage(time_point<high_resolution_clock> start, time_point<high_resolution_clock> end, int level, size_t n){
    Profiler::submit_write_on_storage(level, n);
    if (submitable(&storage_counters[level]->write_counter))
        Profiler::submit_write_on_storage(start, end, level);
}

void ProfilerProxy::submit_ordering(time_point<high_resolution_clock> start, time_point<high_resolution_clock> end){

}

void ProfilerProxy::submit_eviction(time_point<high_resolution_clock> start, time_point<high_resolution_clock> end){

}

CollectedStats* ProfilerProxy::collect(){
    if(service_activated)
        return Profiler::collect();
    return new CollectedStats();
}

bool ProfilerProxy::is_activated(){
    return service_activated;
};