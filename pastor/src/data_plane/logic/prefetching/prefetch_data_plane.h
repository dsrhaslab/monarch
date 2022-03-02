//
// Created by dantas on 26/10/20.
//

#ifndef THESIS_PREFETCH_DATA_PLANE_H
#define THESIS_PREFETCH_DATA_PLANE_H

#include <utility>
#include "../data_plane.h"
#include "../metadata/strict_file_info.h"
#include "../root/hierarchical_data_plane.h"
#include "../../../helpers/logger.h"
#include "../metadata/prefetched_file.h"
#include "../policies.h"

class PrefetchDataPlaneBuilder;

class PrefetchDataPlane : public HierarchicalDataPlane {
    //TODO separate into two pools? one for source and another for local storage
    ctpl::thread_pool* prefetch_thread_pool;
    ReadPolicy read_policy;

public:
    int allocated_samples_index;
    float eviction_percentage;

private:
    explicit PrefetchDataPlane(HierarchicalDataPlane* root_data_plane);
    void start_prefetching_solo_placement();
    void start_prefetching_with_eviction();
    void start_prefetching();
    ssize_t read(StrictFileInfo* mfi, char* result, uint64_t offset, size_t n);

public:
    friend class PrefetchDataPlaneBuilder;
    static PrefetchDataPlaneBuilder* create(HierarchicalDataPlane* root_data_plane);
    ssize_t read(const std::string &filename, char* result, uint64_t offset, size_t n) override;
    ssize_t read_from_id(int file_id, char* result, uint64_t offset, size_t n) override;
    ssize_t read_from_id(int file_id, char* result) override;
    void init(bool transparent_api_) override;
    std::vector<std::string> configs() override;
    void print() override;
    void start() override;
    void debug_write(const std::string& msg) override;
};

#endif //THESIS_PREFETCH_DATA_PLANE_H
