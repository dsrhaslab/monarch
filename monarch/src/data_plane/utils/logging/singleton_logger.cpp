//
// Created by dantas on 19-10-2022.
//

#include "singleton_logger.h"
#include "logger.h"

Logger* SingletonLogger::log_ = nullptr;
std::mutex SingletonLogger::mutex_;

Logger* SingletonLogger::create_instance(const std::string& path, const std::string& log_name, int unique_id){
    std::lock_guard<std::mutex> lock(mutex_);
    if(log_ == nullptr){
        log_ = new Logger();
        log_->configure_service(path, log_name, unique_id);
    }else if (!log_->is_activated()){
        log_->configure_service(path, log_name, unique_id);
    }
    return log_;
}

Logger* SingletonLogger::create_instance(const std::string& path, int unique_id){
    std::lock_guard<std::mutex> lock(mutex_);
    if(log_ == nullptr){
        log_ = new Logger();
        log_->configure_service(path, unique_id);
    } else if (!log_->is_activated()){
        log_->configure_service(path, unique_id);
    }
    return log_;
}

Logger* SingletonLogger::get_instance(){
    std::lock_guard<std::mutex> lock(mutex_);
    if(log_ == nullptr){
        log_ = new Logger();
    }
    return log_;
}
