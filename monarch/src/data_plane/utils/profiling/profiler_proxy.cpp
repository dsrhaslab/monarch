//
// Created by dantas on 19-10-2022.
//

#include "profiler_proxy.h"

ProfilerProxy* ProfilerProxy::instance_ = nullptr;
std::mutex ProfilerProxy::mutex_;

ProfilerProxy* ProfilerProxy::create_instance(int hierarchy_size){
    std::lock_guard<std::mutex> lock(mutex_);
    if(instance_ == nullptr){
        instance_ = new ProfilerProxy(hierarchy_size);
    }else if (!instance_->init_done_){
        instance_->init(hierarchy_size);
    }
    return instance_;
}

ProfilerProxy* ProfilerProxy::create_instance(int hierarchy_size, int update_frequency){
    std::lock_guard<std::mutex> lock(mutex_);
    if(instance_ == nullptr){
        instance_ = new ProfilerProxy(hierarchy_size, update_frequency);
    }else if (!instance_->init_done_){
        instance_->init(hierarchy_size, update_frequency);
    }
    return instance_;
}

ProfilerProxy* ProfilerProxy::create_instance(int hierarchy_size, int update_frequency, int initial_warmup){
    std::lock_guard<std::mutex> lock(mutex_);
    if(instance_ == nullptr){
        instance_ = new ProfilerProxy(hierarchy_size, update_frequency, initial_warmup);
    }else if (!instance_->init_done_){
        instance_->init(hierarchy_size, update_frequency, initial_warmup);
    }
    return instance_;
}

ProfilerProxy* ProfilerProxy::get_instance(){
    std::lock_guard<std::mutex> lock(mutex_);
    if(instance_ == nullptr){
        instance_ = new ProfilerProxy();
    }
    return instance_;
}
