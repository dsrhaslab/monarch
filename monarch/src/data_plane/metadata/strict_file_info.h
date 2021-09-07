//
// Created by dantas on 28/10/20.
//

#ifndef THESIS_STRICT_FILE_INFO_H
#define THESIS_STRICT_FILE_INFO_H

#include "file_info.h"

#include <mutex>
#include <condition_variable>
#include <atomic>

/*Notes:
 * If the file associated to this info is being prefetch, then no other thread is modifying this object
 * Object is readable after the prefetching is done and further modifications like finish_read are thread safe.
*/
//Used for first level only placement
class StrictFileInfo : public FileInfo{
    std::mutex mutex;
    std::condition_variable loaded_condition;
    std::condition_variable unstable_condition;
    //level to where the data is moved/evicted (new origin)
    int staged_level;
    // number of expected reads, also used to prevent multiple prefetching of the same item
    int n_reads;
    //possibly being evicted
    bool unstable;
    //
    int wait_on_loaded;
    int wait_on_unstable;

public:
    void moved_to(int level);
    void loaded_to(int level) override;
    bool await_loaded_data(int target_level);
    bool init_prefetch();
    int finish_read();
    int get_staging_level() const;
    StrictFileInfo(std::string f, size_t size, int origin_level, bool has_shareable_file_descriptors);
    StrictFileInfo(std::string f, size_t size, int origin_level, bool has_shareable_file_descriptors, int target);

};

#endif //THESIS_STRICT_FILE_INFO_H
