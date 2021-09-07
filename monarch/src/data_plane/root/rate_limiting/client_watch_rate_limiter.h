//
// Created by dantas on 04/05/21.
//

#ifndef THESIS_CLIENT_WATCH_RATE_LIMITER_H
#define THESIS_CLIENT_WATCH_RATE_LIMITER_H

#include <mutex>
#include <condition_variable>

//TODO make this for the generic case instead.

class ClientWatchRateLimiter {
    std::mutex mutex;
    std::condition_variable limit_reached;
    std::condition_variable terminated;
    bool rate_enforced;
    int dif_limit_size;
    int samples_placed;
    int client_evicted;
    int total_evictions_expected;

public:
    ClientWatchRateLimiter(int limit_size);
    bool rate_limit();
    void apply_job_start();
    void apply_job_termination();
    void await_termination();
    void set_total_jobs(int total);
};


#endif //THESIS_CLIENT_WATCH_RATE_LIMITER_H
