//
// Created by dantas on 27/02/21.
//

#ifndef THESIS_RATE_LIMITER_H
#define THESIS_RATE_LIMITER_H

#include <mutex>
#include <condition_variable>
#include <atomic>

class RateLimiter {
    std::mutex mutex;
    std::condition_variable limit_reached;
    std::condition_variable terminated;
    int batch_size;
    int started;
    int finished;
    int total;
    int total_finished;

    //last id of a release
    int brake_release_id;
    //last id of a pull
    int brake_id;
    bool hard_brake;

public:
    RateLimiter(int batch_size);
    bool rate_limit();
    void apply_job_termination();
    void pull_brake(int new_brake_id);
    void release_brake(int new_brake_release_id);
    int get_batch_size() const;
    void await_termination();
    void set_total_jobs(int total);
    std::unique_lock<std::mutex> get_lock();
};

#endif //THESIS_RATE_LIMITER_H
