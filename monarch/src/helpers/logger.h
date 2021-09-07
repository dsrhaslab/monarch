//
// Created by dantas on 15/11/20.
//

#ifndef THESIS_LOGGER_H
#define THESIS_LOGGER_H

#include <mutex>
#include <fstream>
#include <unordered_set>

class Logger {
private:
    std::mutex log_mutex;
    std::ofstream log;
    std::string log_path;
    bool configured;
public:
    Logger() = default;
    void configure_service(const std::string& path, int unique_id);
    void _write(const std::string& msg);
    void thread_unsafe_write(const std::string& msg);
    bool is_activated();
};

#endif //THESIS_LOGGER_H
