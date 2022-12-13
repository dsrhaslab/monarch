//
// Created by dantas on 11/09/22.
//

#ifndef MONARCH_SINGLETON_LOGGER_H
#define MONARCH_SINGLETON_LOGGER_H

#include <mutex>
#include <condition_variable>

class Logger;

class SingletonLogger {
private:
    static Logger* log_;
    static std::mutex mutex_;

    SingletonLogger() {};

public:
    SingletonLogger(SingletonLogger& other) = delete;

    void operator=(const SingletonLogger &) = delete;

    static Logger* create_instance(const std::string& path, const std::string& log_name, int unique_id);

    static Logger* create_instance(const std::string& path, int unique_id);

    static Logger* get_instance();
};

#endif //MONARCH_SINGLETON_LOGGER_H