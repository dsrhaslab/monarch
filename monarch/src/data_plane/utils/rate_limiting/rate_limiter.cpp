//
// Created by dantas on 27/02/21.
//
#include "rate_limiter.h"
#include <iostream>

RateLimiter::RateLimiter(int batch_size){
    RateLimiter::batch_size = batch_size;
    RateLimiter::started = 0;
    RateLimiter::finished = 0;
    RateLimiter::hard_brake = false;
    RateLimiter::total_finished = 0;
    RateLimiter::total = 0;
    RateLimiter::brake_id = 0;
    RateLimiter::brake_release_id = 0;
}

bool RateLimiter::rate_limit(){
    bool waited = false;
    std::unique_lock<std::mutex> ul(mutex);
    while (started == batch_size || hard_brake) {
        waited = true;
        limit_reached.wait(ul);
    }
    started++;
    return waited;
}

void RateLimiter::apply_job_termination(){
    std::unique_lock<std::mutex> ul(mutex);
    if(++finished == batch_size){
        started = 0;
        finished = 0;
        if(!hard_brake)
            limit_reached.notify_one();
    }
    if(++total_finished == total)
        terminated.notify_one();
}

//eviction releases break
void RateLimiter::release_brake(int new_release_id){
    /*
    std::unique_lock<std::mutex> ul(mutex);
    if(new_release_id > brake_id) {
        hard_brake = false;
        brake_release_id = new_release_id;
        if(finished == batch_size)
            limit_reached.notify_one();
    }*/
}

void RateLimiter::pull_brake(int new_brake_id){
    /*
    std::unique_lock<std::mutex> ul(mutex);
    //dont accept brakes from the past.
    if(!hard_brake && new_brake_id > brake_id && new_brake_id > brake_release_id) {
        brake_id = new_brake_id;
        hard_brake = true;
    }*/
}

void RateLimiter::set_total_jobs(int total_){
    RateLimiter::total = total_;
}

void RateLimiter::await_termination(){
    std::unique_lock<std::mutex> ul(mutex);
    while(total_finished < total)
        terminated.wait(ul);
}

std::unique_lock<std::mutex> RateLimiter::get_lock(){
    std::unique_lock<std::mutex> ul(mutex);
    return std::move(ul);
}


int RateLimiter::get_batch_size() const{
    return batch_size;
}