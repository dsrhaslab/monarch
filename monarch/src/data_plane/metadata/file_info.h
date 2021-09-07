//
// Created by dantas on 19/10/20.
//

#ifndef THESIS_FILE_INFO_H
#define THESIS_FILE_INFO_H

#include <string>
#include <atomic>
#include <mutex>
#include <tuple>
#include <vector>
#include <memory>

/*
 * File id might change during epochs, but that behavior it's not harmful.
 */

class FileInfo {
private:
    size_t size;

protected:
    int target;
    std::string filename;
    std::atomic<int> storage_level;

    //TODO this should be in another class
    bool shareable_file_descriptors;
    int last_storage_level_read;
    //tuple = [(fd, n_reading, client_is_reading)]
    std::vector<std::tuple<int, int, bool>> descriptors;
    std::unique_ptr<std::mutex> mutex;
public:
    FileInfo(std::string f, size_t size, int origin_level, bool has_shareable_file_descriptors);
    FileInfo(std::string f, size_t size, int origin_level, bool has_shareable_file_descriptors, int t);
    virtual ~FileInfo()= default;

    const std::string &get_name() const;
    size_t _get_size() const;
    int get_storage_level();
    virtual void loaded_to(int level);
    int get_target();
    void set_target(int t);
    bool has_shareable_file_descriptors();
    std::unique_ptr<std::mutex>& get_mutex();
    bool storage_changed();
    int get_last_storage_read() const;
    int get_file_descriptor(int level);
    void update_last_storage_read();
    bool client_is_reading(int level);
    void client_started_read(int level);
    void client_ended_read(int level);
    std::tuple<int, int, bool>& get_descriptor_info(int level);
};

#endif //THESIS_FILE_INFO_H
