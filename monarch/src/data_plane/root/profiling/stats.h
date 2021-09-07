//
// Created by dantas on 17/03/21.
//

#ifndef THESIS_STATS_H
#define THESIS_STATS_H

#include <mutex>
#include <atomic>
#include <vector>
#include <iostream>

struct Stats {
    long count;
    long mean;
    long min;
    long max;
    Stats();
    Stats(Stats const &s) = default;
    void reset();
    std::string get_printable_stats(int tab_level);
    friend std::ostream& operator << (std::ostream &out, const Stats &s);
};

struct MutexStats : public std::mutex {
    Stats content;
};

//fields are all updated at the same time so one mutex is enough
struct MutexClientStats : public std::mutex {
    Stats read_stats;
    Stats wait_stats;
    size_t fetched_bytes;
    MutexClientStats();
    void reset();
};

//for copy
struct ClientStats {
    Stats read_stats;
    Stats wait_stats;
    size_t fetched_bytes;
    ClientStats(MutexClientStats* mcs);
    std::string get_printable_stats(int tab_level);
    friend std::ostream& operator << (std::ostream& out, const ClientStats& s);
};

//many locks for two-phase locking
struct MutexStorageStats {
    MutexStats read_stats;
    MutexStats write_stats;
    MutexStats samples_life_stats;
    MutexStats occupation_stats;
    std::atomic<size_t> read_amount;
    std::atomic<size_t> write_amount;
    MutexStorageStats();
    void reset();
    void lock();
    void unlock();
};

//for copy
struct StorageStats {
    Stats read_stats;
    Stats write_stats;
    Stats samples_life_stats;
    Stats occupation_stats;
    size_t read_amount;
    size_t write_amount;
    StorageStats(MutexStorageStats* mss);
    std::string get_printable_stats(int tab_level);
    friend std::ostream& operator << (std::ostream& out, const StorageStats& s);
};

struct StorageCollectedStats {
    StorageStats collected_stats;
    long time_diff;
    size_t read_bandwidth;
    size_t write_bandwidth;
    StorageCollectedStats(MutexStorageStats* mss);
    void calculate_bandwidth(long time_diff);
    std::string get_printable_stats(int tab_level);
    friend std::ostream& operator << (std::ostream& out, const StorageCollectedStats& s);
};

struct CollectedStats {
    ClientStats* client_stats_snapshot;
    std::vector<StorageCollectedStats*> storage_stats_snapshot;
    friend std::ostream& operator << (std::ostream& out, const CollectedStats& s);
    std::string get_printable_stats();
    CollectedStats();
};

#endif //THESIS_STATS_H
