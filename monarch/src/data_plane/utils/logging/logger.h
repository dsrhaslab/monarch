//
// Created by dantas on 15/11/20.
//

#ifndef MONARCH_LOGGER_H
#define MONARCH_LOGGER_H

#include <mutex>
#include <fstream>
#include <sys/stat.h>
#include <cstring>
#include <iostream>

class Logger {
private:
    std::mutex log_mutex;
    std::ofstream log;
    std::string log_path;
    bool configured = false;
public:
    Logger() = default;

    void configure_service(const std::string& path, const std::string& log_name, int unique_id){
        time_t t = std::time(0);
        auto timestamp = static_cast<long int> (t);
        //TODO doesnt work with recursive dirs...needs other method to create;
        // Create directory
        char tmp[256];
        char *p = nullptr;
        size_t len;

        snprintf(tmp, sizeof(tmp),"%s", path.c_str());
        len = strlen(tmp);
        if(tmp[len - 1] == '/')
            tmp[len - 1] = 0;
        for(p = tmp + 1; *p; p++)
            if(*p == '/') {
                *p = 0;
                mkdir(tmp, S_IRWXU);
                *p = '/';
            }
        mkdir(tmp, S_IRWXU);

        // Open log file
        if (!path.empty()) {
            if(log_name.empty()){
                log_path = path + "/run-" + std::to_string(unique_id) + "-" + std::to_string(timestamp) + ".log";
            }
            else{
                log_path = path + "/" + log_name;
            }
            log.open(log_path, std::ios_base::app);
        } else {
            std::cerr << "Could not find home directory.\n Specific logging will not be performed." << std::endl;
        }
        configured = true;
    }

    void configure_service(const std::string& path, int unique_id){
        configure_service(path, "", unique_id);
    }

    inline void _write(const std::string& msg){
        log_mutex.lock();
        log << msg << std::endl;
        log_mutex.unlock();
    }


    inline void no_lock_write(const std::string& msg){
        log << msg << std::endl;
    }

    inline void _write(const std::string& path, const std::string& msg){
        std::ofstream s_log;
        s_log.open(log_path, std::ios_base::app);
        s_log << msg << std::endl;
        s_log.close();
    }

    inline bool is_activated() const{
        return configured;
    }
};

#endif //MONARCH_LOGGER_H
