//
// Created by dantas on 15/11/20.
//

#include <iostream>
#include <ctime>
#include <sys/stat.h>

#include "logger.h"

void Logger::configure_service(const std::string &path, int unique_id) {
    time_t t = std::time(0);
    auto timestamp = static_cast<long int> (t);
    std::string dir = std::string(path);

    // Create debug directory
    int dir_res = mkdir(dir.c_str(), 0777);
    if (dir_res == 0) {
        std::cout << "Log directory created." << std::endl;
    }

    std::string res;

    // Open log file
    if (!dir.empty()) {
        log_path = std::string(dir) + "/run-" + std::to_string(unique_id) + "-" + std::to_string(timestamp) + ".log";
        log.open(log_path, std::ios_base::app);
    } else {
        std::cerr << "Could not find home directory.\n Specific logging will not be performed." << std::endl;
    }

    configured = true;
}


void Logger::_write(const std::string &msg) {
    log_mutex.lock();
    log << msg << std::endl;
    log_mutex.unlock();
}



void Logger::thread_unsafe_write(const std::string& msg){
    log << msg << std::endl;
}

bool Logger::is_activated(){
    return configured;
}