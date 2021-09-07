//
// Created by dantas on 29/04/21.
//

#ifndef THESIS_PLACED_FILE_INFO_H
#define THESIS_PLACED_FILE_INFO_H

#include "file_info.h"

#include <atomic>

class PlacedFileInfo : public FileInfo {
    std::atomic<bool> placed;

public:
    PlacedFileInfo(std::string f, size_t size, int origin_level, bool has_shareable_file_descriptors) : FileInfo(std::move(f), size, origin_level, has_shareable_file_descriptors), placed(false){}
    PlacedFileInfo(std::string f, size_t size, int origin_level, bool has_shareable_file_descriptors, int target): FileInfo(std::move(f), size, origin_level, has_shareable_file_descriptors, target), placed(false){}
    bool begin_placement(){
        bool expected = false;
        return placed.compare_exchange_strong(expected, true);
    }
};

#endif //THESIS_PLACED_FILE_INFO_H
