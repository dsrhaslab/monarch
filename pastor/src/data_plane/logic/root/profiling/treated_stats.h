//
// Created by dantas on 21/08/21.
//

#ifndef THESIS_TREATED_STATS_H
#define THESIS_TREATED_STATS_H

#include "stats.h"

#define TIME_CONVERSION_CONSTANT 0.001 //millis to seconds

struct TreatedStats {

    float m_bytes_per_second;
    float m_bytes_per_occurrence;
    float m_duration_per_occurrence;

    TreatedStats(){
        m_bytes_per_second = 0;
        m_bytes_per_occurrence = 0;
        m_duration_per_occurrence = 0;
    }

    void reset(){
        m_bytes_per_second = 0;
        m_bytes_per_occurrence = 0;
        m_duration_per_occurrence = 0;
    }

    void treat_stats(Stats s, float time_diff) {
        if(s.n_occurrences > 0) {
            m_bytes_per_second = s.n_bytes / (time_diff * TIME_CONVERSION_CONSTANT);
            m_bytes_per_occurrence = s.n_bytes / s.n_occurrences;
            m_duration_per_occurrence = s.n_duration / s.n_occurrences;
        }
    }

    void add_treated_stats(TreatedStats& ts, uint64_t n_treatments){
        m_bytes_per_second = (m_bytes_per_second * n_treatments + ts.m_bytes_per_second) / (n_treatments + 1);
        m_bytes_per_occurrence = (m_bytes_per_occurrence * n_treatments + ts.m_bytes_per_occurrence) / (n_treatments + 1);
        m_duration_per_occurrence = (m_duration_per_occurrence * n_treatments + ts.m_duration_per_occurrence) / (n_treatments + 1);
    }

    friend std::ostream& operator << (std::ostream &out, const TreatedStats &ts){
        return out << ts.m_bytes_per_second << "," << ts.m_bytes_per_occurrence << "," << ts.m_duration_per_occurrence;
    }
};

struct TreatedClientStats {
    TreatedStats read_stats;
    float full_time_per_occurrence;
    std::unordered_map<std::string, float> metadata_requests_per_second;

    TreatedClientStats(){
        full_time_per_occurrence = 0;
        for(auto& k : {"open", "close"}){
            metadata_requests_per_second.emplace(k, 0.0);
        }
    }

    TreatedClientStats& treat_stats(ClientStats* cs, float time_diff){
        read_stats.treat_stats(cs->read_stats, time_diff);
        if(cs->full_time_stats.n_occurrences > 0) {
            full_time_per_occurrence =
                    (cs->full_time_stats.n_duration) / cs->full_time_stats.n_occurrences;
        }
        for(auto & d : cs->metadata_requests){
            metadata_requests_per_second[std::get<0>(d)] = std::get<1>(d) / (time_diff * TIME_CONVERSION_CONSTANT);
        }
        return *this;
    }

    void reset(){
        read_stats.reset();
        full_time_per_occurrence = 0;
        for(auto& d : metadata_requests_per_second){
            std::get<1>(d) = 0;
        }
    }

    void add_treated_stats(TreatedClientStats& tcs, uint64_t n_treatments) {
        read_stats.add_treated_stats(tcs.read_stats, n_treatments);
        full_time_per_occurrence = (full_time_per_occurrence * n_treatments + tcs.full_time_per_occurrence) / (n_treatments + 1);
        for(auto & d : metadata_requests_per_second){
            std::get<1>(d) = (std::get<1>(d) * n_treatments + tcs.metadata_requests_per_second[std::get<0>(d)]) / (n_treatments + 1);
        }
    }

    static std::string get_headers(){
        std::string res("#");
        res += "open_per_second,close_per_second,";
        return res + "m_bytes_read_per_second,m_bytes_read_per_occurrence,m_duration_per_occurrence,full_time_per_occurrence";
    }

    friend std::ostream& operator << (std::ostream &out, const TreatedClientStats &tcs){
        for(auto & o : {"open", "close"}){
            out << tcs.metadata_requests_per_second.find(o)->second << ",";
        }
        return out << tcs.read_stats << "," << tcs.full_time_per_occurrence;
    }
};

struct TreatedPlacementStats {
    TreatedStats read_stats;
    TreatedStats write_stats;
    float full_time_per_occurrence;
    std::unordered_map<std::string, float> metadata_requests_per_second;

    TreatedPlacementStats(){
        full_time_per_occurrence = 0.0;
        for(auto& k : {"open", "close"}){
            metadata_requests_per_second.emplace(k, 0.0);
        }
    }

    TreatedPlacementStats& treat_stats(PlacementStats* ps, float time_diff){
        read_stats.treat_stats(ps->read_stats, time_diff);
        write_stats.treat_stats(ps->write_stats, time_diff);
        if(ps->full_time_stats.n_occurrences > 0) {
            full_time_per_occurrence =
                    ps->full_time_stats.n_duration / ps->full_time_stats.n_occurrences;
        }
        for(auto & d : ps->metadata_requests){
            metadata_requests_per_second[std::get<0>(d)] = std::get<1>(d) / (time_diff * TIME_CONVERSION_CONSTANT);
        }
        return *this;
    }

    void reset(){
        read_stats.reset();
        write_stats.reset();
        full_time_per_occurrence = 0;
        for(auto& d : metadata_requests_per_second){
            std::get<1>(d) = 0;
        }
    }

    void add_treated_stats(TreatedPlacementStats& tps, uint64_t n_treatments){
        read_stats.add_treated_stats(tps.read_stats, n_treatments);
        write_stats.add_treated_stats(tps.write_stats, n_treatments);
        full_time_per_occurrence = (full_time_per_occurrence * n_treatments + tps.full_time_per_occurrence) / (n_treatments + 1);
        for(auto & d : metadata_requests_per_second){
            std::get<1>(d) = (std::get<1>(d) * n_treatments + tps.metadata_requests_per_second[std::get<0>(d)]) / (n_treatments + 1);
        }
    }

    static std::string get_headers(){
        std::string res("#");
        res += "open_per_second,close_per_second,";
        res += "m_bytes_read_per_second,m_bytes_read_per_occurrence,m_read_duration_per_occurrence";
        return res + "m_bytes_write_per_second,m_bytes_write_per_occurrence,m_write_duration_per_occurrence,full_time_per_occurrence";
    }

    friend std::ostream& operator << (std::ostream &out, const TreatedPlacementStats &tps){
        for(auto & o : {"open", "close"}){
            out << tps.metadata_requests_per_second.find(o)->second << ",";
        }
        out << tps.read_stats;
        return out << tps.write_stats << "," << tps.full_time_per_occurrence;
    }
};

struct TreatedStorageStats {
    TreatedStats client_read_stats;
    TreatedStats background_read_stats;
    TreatedStats aggregate_read_stats;
    TreatedStats write_stats;
    std::unordered_map<std::string, float> metadata_requests_per_second;

    TreatedStorageStats(){
        for(auto& k : {"open", "close"}){
            metadata_requests_per_second.emplace(k, 0.0);
        }
    }

    TreatedStorageStats& treat_stats(StorageStats* ss, float time_diff){
        client_read_stats.treat_stats(ss->client_read_stats, time_diff);
        background_read_stats.treat_stats(ss->background_read_stats, time_diff);
        aggregate_read_stats.treat_stats(ss->aggregate_read_stats, time_diff);
        write_stats.treat_stats(ss->write_stats, time_diff);
        for(auto & d : ss->metadata_requests){
            metadata_requests_per_second[std::get<0>(d)] = std::get<1>(d) / (time_diff * TIME_CONVERSION_CONSTANT);
        }
        return *this;
    }

    void reset(){
        client_read_stats.reset();
        background_read_stats.reset();
        aggregate_read_stats.reset();
        write_stats.reset();
        for(auto& d : metadata_requests_per_second){
            std::get<1>(d) = 0;
        }
    }

    void add_treated_stats(TreatedStorageStats& tss, uint64_t n_treatments){
        client_read_stats.add_treated_stats(tss.client_read_stats, n_treatments);
        background_read_stats.add_treated_stats(tss.background_read_stats, n_treatments);
        aggregate_read_stats.add_treated_stats(tss.aggregate_read_stats, n_treatments);
        write_stats.add_treated_stats(tss.write_stats, n_treatments);
        for(auto & d : metadata_requests_per_second){
            std::get<1>(d) = (std::get<1>(d) * n_treatments + tss.metadata_requests_per_second[std::get<0>(d)]) / (n_treatments + 1);
        }
    }

    static std::string get_headers(){
        std::string res("#");
        res += "open_per_second,close_per_second,";
        res += "m_client_bytes_read_per_second,m_client_bytes_read_per_occurrence,m_client_read_duration_per_occurrence,";
        res += "m_background_bytes_read_per_second,m_background_bytes_read_per_occurrence,m_background_read_duration_per_occurrence,";
        res += "m_aggregate_bytes_read_per_second,m_aggregate_bytes_read_per_occurrence,m_aggregate_read_duration_per_occurrence,";
        res += "m_bytes_written_per_second,m_bytes_written_per_occurrence,m_write_duration_per_occurrence";
        return res;
    }

    friend std::ostream& operator << (std::ostream& out, const TreatedStorageStats& tss){
        for(auto & o : {"open", "close"}){
            out << tss.metadata_requests_per_second.find(o)->second << ",";
        }
        return out << tss.client_read_stats << "," << tss.background_read_stats << ","
        << tss.aggregate_read_stats << "," << tss.write_stats;
    }
};

struct TreatedCollectedStats {
    TreatedClientStats client_stats_snapshot;
    TreatedPlacementStats background_stats_snapshot;
    std::vector<TreatedStorageStats> storage_stats_snapshot;

    TreatedCollectedStats() = default;

    explicit TreatedCollectedStats(CollectedStats& cs){
        client_stats_snapshot.treat_stats(cs.client_stats_snapshot, cs.time_diff);
        background_stats_snapshot.treat_stats(cs.background_stats_snapshot, cs.time_diff);
        for(int i = 0; i < cs.storage_stats_snapshot.size(); i++){
            storage_stats_snapshot.emplace_back();
            storage_stats_snapshot[i].treat_stats(cs.storage_stats_snapshot[i], cs.time_diff);
        }
    }
};

struct AverageCollectedStats : public TreatedCollectedStats {
    uint64_t n_treatments;

    AverageCollectedStats(){
        n_treatments = 0;
    }

    void reset(){
        client_stats_snapshot.reset();
        background_stats_snapshot.reset();
        for(auto& st : storage_stats_snapshot){
            st.reset();
        }
    }

    void add_treated_stats(TreatedCollectedStats& tcs){
        client_stats_snapshot.add_treated_stats(tcs.client_stats_snapshot, n_treatments);
        background_stats_snapshot.add_treated_stats(tcs.background_stats_snapshot, n_treatments);
        for (int i = 0; i < tcs.storage_stats_snapshot.size(); i++) {
            if(n_treatments == 0) {
                storage_stats_snapshot.emplace_back();
            }
            storage_stats_snapshot[i].add_treated_stats(tcs.storage_stats_snapshot[i], n_treatments);
        }
        n_treatments++;
    }
};

#endif //THESIS_TREATED_STATS_H
