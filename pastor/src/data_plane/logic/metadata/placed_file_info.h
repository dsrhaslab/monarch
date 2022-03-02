//
// Created by dantas on 29/04/21.
//

#ifndef THESIS_PLACED_FILE_INFO_H
#define THESIS_PLACED_FILE_INFO_H

#include "file_info.h"

#include <atomic>

class PlacedFileInfo : public FileInfo {
    //placed: true && in_place: false > is in the state of being placed
    //placed: true && in_place: true > finished placement
    //placed: false -> was never placed (e.g, storage full)
    std::atomic<bool> placed;
    //bool in_place;

public:
    PlacedFileInfo(std::string f, size_t size, int origin_level, bool has_shareable_file_descriptors)
        : FileInfo(std::move(f), size, origin_level, has_shareable_file_descriptors),
        placed(false){}

    PlacedFileInfo(std::string f, size_t size, int origin_level, bool has_shareable_file_descriptors, int target)
        : FileInfo(std::move(f), size, origin_level, has_shareable_file_descriptors, target),
        placed(false){}

    bool begin_placement(){
        bool expected = false;
        return placed.compare_exchange_strong(expected, true);
    }
   /*
    void end_placement(){
        in_place = true;
    }

    //This assumes that the "read" is issued before "close", thus making at least the placed == true.
    bool client_close_required(){
        return (placed && in_place) || !placed;
    }
    */
};

#endif //THESIS_PLACED_FILE_INFO_H
