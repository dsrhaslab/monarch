//
// Created by dantas on 04/05/21.
//

#include "client_watch_rate_limiter.h"

ClientWatchRateLimiter::ClientWatchRateLimiter(int limit_size){
    dif_limit_size = limit_size;
    samples_placed = 0;
    client_evicted = 0;
    total_evictions_expected = 0;
    rate_enforced = false;
}

bool ClientWatchRateLimiter::rate_limit(){
    bool waited = false;
    std::unique_lock<std::mutex> ul(mutex);
    //by design client_evicted cannot be bigger than prefetch_started;
    while(samples_placed - client_evicted >= dif_limit_size) {
        rate_enforced = true;
        waited = true;
        limit_reached.wait(ul);
    }
    return waited;
}

void ClientWatchRateLimiter::apply_job_start(){
    std::unique_lock<std::mutex> ul(mutex);
    samples_placed++;
}

void ClientWatchRateLimiter::apply_job_termination(){
    std::unique_lock<std::mutex> ul(mutex);
    if(samples_placed - client_evicted++ < dif_limit_size && rate_enforced){
        rate_enforced = false;
        ul.unlock();
        limit_reached.notify_one();
    }
    if(client_evicted == total_evictions_expected)
        terminated.notify_one();
}

void ClientWatchRateLimiter::await_termination(){
    std::unique_lock<std::mutex> ul(mutex);
    while(client_evicted < total_evictions_expected)
        terminated.wait(ul);
}

void ClientWatchRateLimiter::set_total_jobs(int total){
    total_evictions_expected = total;
}