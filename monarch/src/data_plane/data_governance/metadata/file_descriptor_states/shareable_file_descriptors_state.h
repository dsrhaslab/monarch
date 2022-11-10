//
// Created by dantas on 17-06-2022.
//

#ifndef MONARCH_SHAREABLE_FILE_DESCRIPTORS_STATE_H
#define MONARCH_SHAREABLE_FILE_DESCRIPTORS_STATE_H

#include <vector>
#include <memory>
#include <mutex>

#include "file_descriptors_state.h"

class ShareableFileDescriptorsState : public FileDescriptorsState {
private:
    std::unique_ptr<std::mutex> mutex;
    int last_storage_level_read;
    //(fd, n_clients_reading)
    std::vector<std::tuple<int, int>> descriptors;

public:
    explicit ShareableFileDescriptorsState(int origin_level){
        mutex = std::make_unique<std::mutex>();
        descriptors.reserve(origin_level + 1);
        for (int i = 0; i <= origin_level; i++) {
            descriptors.emplace_back(-1, 0);
        }
        last_storage_level_read = origin_level;
    }

    int get_file_descriptor(int level) override{
        return std::get<0>(descriptors[level]);
    }

    void set_file_descriptor(int value, int level) override{
        std::get<0>(descriptors[level]) = value;
    }

    FileDescriptorsStateType get_type() override{
        return FileDescriptorsStateType::SHAREABLE;
    };

    bool storage_changed(int current_storage_level) const{
        return last_storage_level_read != current_storage_level;
    }

    int get_last_storage_read() const{
        return last_storage_level_read;
    }

    int update_last_storage_read(int storage_level){
        last_storage_level_read = storage_level;
        return storage_level;
    }

    std::tuple<int, int>& get_descriptors_info(int level){
        return descriptors[level];
    }

    std::tuple<int, int>& get_descriptors_info_lsr(){
        return descriptors[last_storage_level_read];
    }

    std::unique_ptr<std::mutex>& get_mutex(){
        return mutex;
    }
};

#endif //MONARCH_SHAREABLE_FILE_DESCRIPTORS_STATE_H
