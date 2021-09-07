//
// Created by dantas on 17/03/21.
//

#include <climits>
#include <sstream>

#include "stats.h"

Stats::Stats() {reset();}

void Stats::reset(){
    count = 0;
    mean = 0;
    min = LONG_MAX;
    max = -1;
}

std::ostream& operator<<(std::ostream &out, const Stats &s) {
    return out << s.count << "," << s.mean << "," << s.min << "," << s.max;
}

std::string Stats::get_printable_stats(int tab_level) {
    std::string tabs(tab_level, '\t');
    return tabs + "Number of measurements: " + std::to_string(count) + "\n" +
           tabs + "Mean: " + std::to_string(mean) + "\n" +
           tabs + "Min: "  + std::to_string(min)  + "\n" +
           tabs + "Max: "  + std::to_string(max)  + "\n";
}

MutexClientStats::MutexClientStats() {
    fetched_bytes = 0;
}

void MutexClientStats::reset() {
    read_stats.reset();
    wait_stats.reset();
    fetched_bytes = 0;
}

ClientStats::ClientStats(MutexClientStats* mcs) :
        read_stats(mcs->read_stats), wait_stats(mcs->wait_stats), fetched_bytes(mcs->fetched_bytes){}

std::ostream& operator<<(std::ostream& out, const ClientStats& s) {
    return out << s.read_stats << "," << s.wait_stats << "," << s.fetched_bytes;
}

std::string ClientStats::get_printable_stats(int tab_level) {
    std::string tabs(tab_level, '\t');
    return tabs + "Read statistics ->\n" +
           read_stats.get_printable_stats(tab_level + 1) +
           tabs + "Wait statistics ->\n" +
           wait_stats.get_printable_stats(tab_level + 1) +
           tabs + "total fetched bytes: " + std::to_string(fetched_bytes) + "\n";
}

MutexStorageStats::MutexStorageStats(){
    read_amount = 0;
    write_amount = 0;
}

void MutexStorageStats::reset() {
    read_stats.content.reset();
    write_stats.content.reset();
    samples_life_stats.content.reset();
    occupation_stats.content.reset();
    read_amount = 0;
    write_amount = 0;
}

void MutexStorageStats::lock(){
    read_stats.lock();
    write_stats.lock();
    samples_life_stats.lock();
    occupation_stats.lock();
}

void MutexStorageStats::unlock(){
    read_stats.unlock();
    write_stats.unlock();
    samples_life_stats.unlock();
    occupation_stats.unlock();
}

StorageStats::StorageStats(MutexStorageStats* mss) :
        read_stats(mss->read_stats.content), write_stats(mss->write_stats.content),
        samples_life_stats(mss->samples_life_stats.content), occupation_stats(mss->occupation_stats.content),
        read_amount(mss->read_amount), write_amount(mss->write_amount) {}

std::ostream &operator<<(std::ostream &out, const StorageStats &s) {
    return out << s.read_stats << "," << s.read_amount
               << s.write_stats << "," << s.write_amount
               << s.samples_life_stats << "," << s.occupation_stats;
}

std::string StorageStats::get_printable_stats(int tab_level) {
    std::string tabs(tab_level, '\t');
    return tabs + "Read statistics ->\n" +
           read_stats.get_printable_stats(tab_level + 1) +
           tabs + "Total read amount: " + std::to_string(read_amount) + "\n" +
           tabs + "Write statistics ->\n" +
           write_stats.get_printable_stats(tab_level + 1) +
           tabs + "Total write amount: " + std::to_string(write_amount) + "\n" +
           tabs + "Samples life statistics ->\n" +
           samples_life_stats.get_printable_stats(tab_level + 1) +
           tabs + "Occupation stats ->\n" +
           occupation_stats.get_printable_stats(tab_level + 1);
}

StorageCollectedStats::StorageCollectedStats(MutexStorageStats* mss) : collected_stats(mss) {
    time_diff = 0;
    read_bandwidth = 0;
    write_bandwidth = 0;
}

void StorageCollectedStats::calculate_bandwidth(long tf) {
    StorageCollectedStats::time_diff = tf;
    StorageCollectedStats::read_bandwidth = collected_stats.read_amount / tf * 1000;
    StorageCollectedStats::write_bandwidth = collected_stats.write_amount / tf * 1000;
}

std::ostream &operator<<(std::ostream &out, const StorageCollectedStats &s) {
    return out << s.collected_stats << "," << s.time_diff << "," << s.read_bandwidth << "," << s.write_bandwidth;
}

std::string StorageCollectedStats::get_printable_stats(int tab_level) {
    std::string tabs(tab_level, '\t');
    return tabs + "Storage collected stats ->\n" +
           collected_stats.get_printable_stats(tab_level + 1) +
           tabs + "Time difference from last checkpoint: " + std::to_string(time_diff) + "\n" +
           tabs + "Read bandwidth: " + std::to_string(read_bandwidth) + "\n" +
           tabs + "Write bandwidth: " + std::to_string(write_bandwidth) + "\n";
}

std::ostream &operator<<(std::ostream &out, const CollectedStats &s) {
    out << s.client_stats_snapshot;
    for(auto& storage_collected_stats : s.storage_stats_snapshot)
    out << "," << storage_collected_stats;
    return out;
}

//TODO fix this by zeroing out all fields
CollectedStats::CollectedStats(){
    client_stats_snapshot = new ClientStats(new MutexClientStats());
}

std::string CollectedStats::get_printable_stats() {
    std::stringstream storage_printables;
    for(int i = 0; i < storage_stats_snapshot.size(); i++)
        storage_printables << "\tDriver -> " << std::to_string(i) << "\n" <<
        storage_stats_snapshot[i]->get_printable_stats(2);

    return  "Client statistics snapshot ->\n" + client_stats_snapshot->get_printable_stats(1) +
            "Storage hierarchy statistics snapshot ->\n" + storage_printables.str();
}