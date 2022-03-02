//
// Created by dantas on 17/03/21.
//

#ifndef THESIS_STATS_H
#define THESIS_STATS_H

#include <mutex>
#include <atomic>
#include <vector>
#include <iostream>
#include <sstream>
#include <unordered_map>

//WARNING! This header's methods are not thread-safe. Locking must be explicitly made by the caller.

//stats collected on a time interval (not in function of time and not treated)

struct Duration {
    uint64_t n_occurrences;
    //accumulated duration of traced operation
    float n_duration;

    Duration(){
        n_occurrences = 0;
        n_duration = 0.0;
    }

    virtual void reset(){
        n_occurrences = 0;
        n_duration = 0.0;
    }

    virtual void add(Duration& d){
        n_occurrences += d.n_occurrences;
        n_duration += d.n_duration;
    }

    friend std::ostream& operator << (std::ostream &out, const Duration &s){
        return out << s.n_duration;
    }
};

struct MutexDuration : public std::mutex {
    Duration content;
};

struct Stats : public Duration {
    uint64_t n_bytes;

    Stats() : Duration(){
        n_bytes = 0;
    }

    Stats(Stats const &s) = default;

    void reset() override{
        Duration::reset();
        n_bytes = 0;
    }

    void add(Stats& s){
        Duration::add(s);
        n_bytes += s.n_bytes;
    }

    friend std::ostream& operator << (std::ostream &out, const Stats &s){
        return out << s.n_occurrences << "," << s.n_bytes << "," << s.n_duration;
    }
};

struct MutexStats : public std::mutex {
    Stats content;
};

//fields are all updated at the same time so one mutex is enough
struct MutexClientStats{
    MutexStats read_stats;
    MutexDuration full_time_stats;
    std::unordered_map<std::string, std::atomic<uint64_t>> metadata_requests;

    MutexClientStats(){
        for(auto& k : {"open", "close"}){
            metadata_requests.emplace(k, 0);
        }
    }

    void reset(){
        read_stats.content.reset();
        full_time_stats.content.reset();
        for(auto & d : metadata_requests){
            std::get<1>(d) = 0;
        }
    }

    void lock(){
        read_stats.lock();
        full_time_stats.lock();
    }

    void unlock(){
        read_stats.unlock();
        full_time_stats.unlock();
    }
};

//for copy
struct ClientStats {
    Stats read_stats;
    Duration full_time_stats;
    std::unordered_map<std::string, uint64_t> metadata_requests;

    ClientStats(){
        for(auto& k : {"open", "close"}){
            metadata_requests.emplace(k, 0);
        }
    }

    ClientStats(MutexClientStats* mcs) : read_stats(mcs->read_stats.content), full_time_stats(mcs->full_time_stats.content){
        for(auto & d : mcs->metadata_requests){
            uint64_t v = std::get<1>(d);
            metadata_requests.insert(std::make_pair(std::get<0>(d), v));
        }
    }

    void reset(){
        read_stats.reset();
        full_time_stats.reset();
        for(auto & d : metadata_requests){
            std::get<1>(d) = 0;
        }
    }

    void add(ClientStats* cs){
        read_stats.add(cs->read_stats);
        full_time_stats.add(cs->full_time_stats);
        for(auto& d : metadata_requests){
            std::get<1>(d) += cs->metadata_requests[std::get<0>(d)];
        }
    }

    static std::string get_headers() {
        std::string res("#");
        res.append("open,close,");
        res.append("n_reads,n_bytes_read,n_duration_read,full_duration");
        return res;
    }

    friend std::ostream& operator << (std::ostream& out, const ClientStats& cs){
        for(auto & o : {"open", "close"}){
            out << cs.metadata_requests.find(o)->second << ",";
        }
        return out << cs.read_stats << "," << cs.full_time_stats;
    }
};

//how much time (read + write)
//how many files placed per second
struct MutexPlacementStats {
    MutexStats read_stats;
    MutexStats write_stats;
    MutexDuration full_time_stats;
    std::unordered_map<std::string, std::atomic<uint64_t>> metadata_requests;

    MutexPlacementStats(){
        for(auto& k : {"open", "close"}){
            metadata_requests.emplace(k, 0);
        }
    }

    void reset(){
        read_stats.content.reset();
        write_stats.content.reset();
        full_time_stats.content.reset();
        for(auto & d : metadata_requests){
            std::get<1>(d) = 0;
        }
    }

    void lock(){
        read_stats.lock();
        write_stats.lock();
        full_time_stats.lock();
    }

    void unlock(){
        read_stats.unlock();
        write_stats.unlock();
        full_time_stats.unlock();
    }
};

struct PlacementStats {
    Stats read_stats;
    Stats write_stats;
    Duration full_time_stats;
    std::unordered_map<std::string, uint64_t> metadata_requests;

    PlacementStats(){
        for(auto& k : {"open", "close"}){
            metadata_requests.emplace(k, 0);
        }
    }

    explicit PlacementStats(MutexPlacementStats* mps)
        : read_stats(mps->read_stats.content), write_stats(mps->write_stats.content), full_time_stats(mps->full_time_stats.content) {
        for(auto & d : mps->metadata_requests){
            uint64_t v = std::get<1>(d);
            metadata_requests.insert(std::make_pair(std::get<0>(d), v));
        }
    }

    void reset(){
        read_stats.reset();
        write_stats.reset();
        full_time_stats.reset();
        for(auto & d : metadata_requests){
            std::get<1>(d) = 0;
        }
    }

    void add(PlacementStats* ps){
        read_stats.add(ps->read_stats);
        write_stats.add(ps->write_stats);
        full_time_stats.add(ps->full_time_stats);
        for(auto& d : metadata_requests){
            std::get<1>(d) += ps->metadata_requests[std::get<0>(d)];
        }
    }

    static std::string get_headers() {
        std::string res("#");
        res += "open,close,";
        return res + "n_reads,n_bytes_read,n_duration_read,n_writes,n_bytes_written,n_duration_write,full_duration";
    }

    friend std::ostream& operator << (std::ostream& out, const PlacementStats& ps){
        for(auto & o : {"open", "close"}){
            out << ps.metadata_requests.find(o)->second << ",";
        }
        return out << ps.read_stats << "," << ps.write_stats << "," << ps.full_time_stats;
    }
};


//many locks for two-phase locking
struct MutexStorageStats {
    //aggregate hold the lock for the client + background
    Stats client_read_stats;
    Stats background_read_stats;
    MutexStats aggregate_read_stats;
    MutexStats write_stats;
    std::unordered_map<std::string, uint64_t> metadata_requests;

    MutexStorageStats(){
        for(auto& k : {"open", "close"}){
            metadata_requests.emplace(k, 0);
        }
    }

    void reset(){
        client_read_stats.reset();
        background_read_stats.reset();
        aggregate_read_stats.content.reset();
        write_stats.content.reset();
        for(auto & d : metadata_requests){
            std::get<1>(d) = 0;
        }
    }

    void lock(){
        aggregate_read_stats.lock();
        write_stats.lock();
    }

    void unlock(){
        aggregate_read_stats.unlock();
        write_stats.unlock();
    }
};

//for copy
struct StorageStats {
    Stats client_read_stats;
    Stats background_read_stats;
    Stats aggregate_read_stats;
    Stats write_stats;
    std::unordered_map<std::string, uint64_t> metadata_requests;

    StorageStats(){
        for(auto& k : {"open", "close"}){
            metadata_requests.emplace(k, 0);
        }
    }

    explicit StorageStats(MutexStorageStats* mss)
        : client_read_stats(mss->client_read_stats), background_read_stats(mss->background_read_stats),
        aggregate_read_stats(mss->aggregate_read_stats.content), write_stats(mss->write_stats.content){

        for(auto & d : mss->metadata_requests){
            uint64_t v = std::get<1>(d);
            metadata_requests.insert(std::make_pair(std::get<0>(d), v));
        }
    }

    void reset(){
        client_read_stats.reset();
        background_read_stats.reset();
        aggregate_read_stats.reset();
        write_stats.reset();
        for(auto& d : metadata_requests){
            std::get<1>(d) = 0;
        }
    }

    void add(StorageStats* ss){
        client_read_stats.add(ss->client_read_stats);
        background_read_stats.add(ss->background_read_stats);
        aggregate_read_stats.add(ss->aggregate_read_stats);
        write_stats.add(ss->write_stats);
        for(auto& d : metadata_requests){
            std::get<1>(d) += ss->metadata_requests[std::get<0>(d)];
        }
    }

    static std::string get_headers(){
        std::string res("#");
        res += "open,close,";
        res += "n_client_reads,n_client_bytes_read,n_client_duration_read,";
        res += "n_background_reads,n_background_bytes_read,n_background_duration_read,";
        res += "n_aggregate_reads,n_aggregate_bytes_read,n_aggregate_duration_read";
        res += "n_writes,n_bytes_written,n_duration_written";
        return res;
    }

    friend std::ostream& operator << (std::ostream& out, const StorageStats& ss){
        for(auto & o : {"open", "close"}){
            out << ss.metadata_requests.find(o)->second << ",";
        }
        return out << ss.client_read_stats << "," << ss.background_read_stats << ","
               << ss.aggregate_read_stats << "," << ss.write_stats;
    }
};

struct CollectedStats {
    ClientStats* client_stats_snapshot;
    PlacementStats* background_stats_snapshot;
    std::vector<StorageStats*> storage_stats_snapshot;
    float time_diff;

    CollectedStats(){
        time_diff = 0;
    }

    void reset(){
        client_stats_snapshot->reset();
        background_stats_snapshot->reset();
        for(auto* st : storage_stats_snapshot){
            st->reset();
        }
    }

    void add(CollectedStats& cs){
        client_stats_snapshot->add(cs.client_stats_snapshot);
        background_stats_snapshot->add(cs.background_stats_snapshot);
        if(!storage_stats_snapshot.empty()){
            for(int i = 0; i < storage_stats_snapshot.size(); i++){
                storage_stats_snapshot[i]->add(cs.storage_stats_snapshot[i]);
            }
        }else{
            for(int i = 0; i < cs.storage_stats_snapshot.size(); i++){
                storage_stats_snapshot.emplace_back();
                storage_stats_snapshot[i]->add(cs.storage_stats_snapshot[i]);
            }
        }
        time_diff += cs.time_diff;
    }
};

#endif //THESIS_STATS_H
