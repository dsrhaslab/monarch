//
// Created by dantas on 20/08/21.
//

#ifndef THESIS_PROFILING_SERVICE_H
#define THESIS_PROFILING_SERVICE_H

#if defined BAZEL_BUILD || defined TF_BAZEL_BUILD
#include "third_party/ctpl/ctpl.h"
#else
#include <ctpl.h>
#endif

#include "profiler_proxy.h"
#include "treated_stats.h"
#include "../../../../helpers/logger.h"

#define CLIENT_SUFFIX "/client"
#define PLACEMENT_SUFFIX "/placement"
#define STORAGE_SUFFIX "/storage"

#define RAW_FILENAME "raw_stats"
#define TREATED_FILENAME "treated_stats"


class ProfilingService {
private:
    ProfilerProxy* profiler;
    int collect_frequency_;

    std::string profiling_root_dir;
    std::string time_series_dir;
    std::string epochs_dir;
    std::string global_dir;

    std::unordered_map<std::string, Logger*> loggers;

    AverageCollectedStats epoch_average_stats;
    CollectedStats epoch_accumulated_stats;
    CollectedStats global_accumulated_stats;

    ctpl::thread_pool* handler;

    std::mutex mutex;
    std::condition_variable has_finished_epoch_tasks;
    int epoch_background_tasks;
    bool waiting_on_completion;

private:

    void write(const std::string& path, const std::string& msg){
        loggers[path]->no_lock_write(msg);
    }

    static std::string get_full_path(const std::string& dir, const std::string& filename){
        return dir + "/" + filename;
    }

    void configure_logger(const std::string& dir, const std::string& filename){
        std::string crd = get_full_path(dir, filename);
        loggers.emplace(crd, new Logger());
        loggers[crd]->configure_service(dir, filename, 0);
    }

    void configure_loggers(const std::string& prefix, const std::string& name){
        configure_logger(prefix + CLIENT_SUFFIX, name);
        configure_logger(prefix + PLACEMENT_SUFFIX, name);
        for(int i = 0; i < profiler->hierarchy_size_; i++){
            configure_logger(prefix + STORAGE_SUFFIX, name + "-s" + std::to_string(i));
        }
    }

    void flush_raw_headers(const std::string& prefix){
        write(get_full_path(prefix + CLIENT_SUFFIX, RAW_FILENAME), ClientStats::get_headers());
        write(get_full_path(prefix + PLACEMENT_SUFFIX, RAW_FILENAME), PlacementStats::get_headers());
        for(int i = 0; i < profiler->hierarchy_size_; i++){
            write(get_full_path(prefix + STORAGE_SUFFIX, std::string(RAW_FILENAME) + "-s" + std::to_string(i)), StorageStats::get_headers());
        }
    }

    void flush_treated_headers(const std::string& prefix){
        write(get_full_path(prefix + CLIENT_SUFFIX, TREATED_FILENAME), TreatedClientStats::get_headers());
        write(get_full_path(prefix + PLACEMENT_SUFFIX, TREATED_FILENAME), TreatedPlacementStats::get_headers());
        for(int i = 0; i < profiler->hierarchy_size_; i++){
            write(get_full_path(prefix + STORAGE_SUFFIX, std::string(TREATED_FILENAME) + "-s" + std::to_string(i)), TreatedStorageStats::get_headers());
        }
    }

    void flush_entry(const std::string& prefix, const std::string& name, CollectedStats& cs){
        std::ostringstream stream;

        stream << *cs.client_stats_snapshot;
        write(get_full_path(prefix + CLIENT_SUFFIX, name), stream.str());
        stream.str(std::string());

        stream << *cs.background_stats_snapshot;
        write(get_full_path(prefix + PLACEMENT_SUFFIX, name), stream.str());
        stream.str(std::string());

        for(int i = 0; i < cs.storage_stats_snapshot.size(); i++){
            stream << *cs.storage_stats_snapshot[i];
            write(get_full_path(prefix + STORAGE_SUFFIX, name  + "-s" + std::to_string(i)), stream.str());
            stream.str(std::string());
        }
    }

    void flush_entry(const std::string& prefix, const std::string& name, TreatedCollectedStats& tcs){
        std::ostringstream stream;

        stream << tcs.client_stats_snapshot;
        write(get_full_path(prefix + CLIENT_SUFFIX, name), stream.str());
        stream.str(std::string());

        stream << tcs.background_stats_snapshot;
        write(get_full_path(prefix + PLACEMENT_SUFFIX, name), stream.str());
        stream.str(std::string());

        for(int i = 0; i < tcs.storage_stats_snapshot.size(); i++){
            stream << tcs.storage_stats_snapshot[i];
            write(get_full_path(prefix + STORAGE_SUFFIX, name + "-s" + std::to_string(i)), stream.str());
            stream.str(std::string());
        }
    }

    void collect_epoch(){
        flush_entry(epochs_dir, RAW_FILENAME, epoch_accumulated_stats);
        flush_entry(epochs_dir, TREATED_FILENAME, epoch_average_stats);

        epoch_accumulated_stats.reset();
        epoch_average_stats.reset();

        std::unique_lock<std::mutex> ul(mutex);
        epoch_background_tasks--;
        if(epoch_background_tasks == 0 && waiting_on_completion){
            has_finished_epoch_tasks.notify_all();
        }
    }

    void collect(){
        sleep(collect_frequency_);

        CollectedStats collected_stats;
        profiler->collect(collected_stats);

        flush_entry(time_series_dir, RAW_FILENAME, collected_stats);

        epoch_accumulated_stats.add(collected_stats);
        global_accumulated_stats.add(collected_stats);

        auto treated_collected_stats = TreatedCollectedStats(collected_stats);
        flush_entry(time_series_dir, TREATED_FILENAME, treated_collected_stats);

        epoch_average_stats.add_treated_stats(treated_collected_stats);

        handler->push([this](int id){
            collect();
        });
    }


public:
    ProfilingService(const std::string& path, int collect_frequency, ProfilerProxy* profiler_){
        profiler = profiler_;
        collect_frequency_ = collect_frequency;

        time_t t = std::time(0);
        auto timestamp = static_cast<long int> (t);
        profiling_root_dir = path + "/run-" + std::to_string(timestamp);

        time_series_dir = profiling_root_dir + "/time_series";
        epochs_dir = profiling_root_dir + "/epochs";
        global_dir = profiling_root_dir + "/global";

        configure_loggers(time_series_dir, RAW_FILENAME);
        configure_loggers(time_series_dir, TREATED_FILENAME);
        flush_raw_headers(time_series_dir);
        flush_treated_headers(time_series_dir);

        configure_loggers(epochs_dir, RAW_FILENAME);
        configure_loggers(epochs_dir, TREATED_FILENAME);
        flush_raw_headers(epochs_dir);
        flush_treated_headers(epochs_dir);

        configure_loggers(global_dir, RAW_FILENAME);
        flush_raw_headers(global_dir);

        handler = new ctpl::thread_pool(1);

        epoch_accumulated_stats.client_stats_snapshot = new ClientStats();
        epoch_accumulated_stats.background_stats_snapshot = new PlacementStats();
        global_accumulated_stats.client_stats_snapshot = new ClientStats();
        global_accumulated_stats.background_stats_snapshot = new PlacementStats();

        for(int i = 0; i < profiler_->hierarchy_size_; i++){
            epoch_accumulated_stats.storage_stats_snapshot.push_back(new StorageStats());
            global_accumulated_stats.storage_stats_snapshot.push_back(new StorageStats());
        }

        epoch_background_tasks = 0;
        waiting_on_completion = false;
    }

    ~ProfilingService(){
        flush_entry(global_dir, RAW_FILENAME, global_accumulated_stats);
        std::unique_lock<std::mutex> ul(mutex);
        while(epoch_background_tasks > 0){
            waiting_on_completion = true;
            has_finished_epoch_tasks.wait(ul);
        }
    }

    void signal_finished_epoch(){
        std::unique_lock<std::mutex> ul(mutex);
        epoch_background_tasks++;
        ul.unlock();
        handler->push([this](int id){
            collect_epoch();
        });
    }

    void start(){
        handler->push([this](int id){
            collect();
        });
    }

};

#endif //THESIS_PROFILING_SERVICE_H
