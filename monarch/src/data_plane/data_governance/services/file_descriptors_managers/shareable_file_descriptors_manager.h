//
// Created by dantas on 15-10-2022.
//

#ifndef MONARCH_SHAREABLE_FILE_DESCRIPTORS_MANAGER_H
#define MONARCH_SHAREABLE_FILE_DESCRIPTORS_MANAGER_H

#include <functional>

#include "../../metadata/info.h"
#include "../../../utils/logging/singleton_logger.h"


class ShareableFileDescriptorsManager {
public:
    static inline int client_submit_open(Info* i, int storage_level, const std::function<int()>& open_call){
        auto* shareable_fd_state = static_cast<ShareableFileDescriptorsState*>(i->file_descriptor_state);
        auto& descriptors_info = shareable_fd_state->get_descriptors_info(storage_level);
        std::lock_guard<std::mutex> lock(*shareable_fd_state->get_mutex());
        std::get<1>(descriptors_info)++;
        auto& file_descriptor = std::get<0>(descriptors_info);
        shareable_fd_state->update_last_storage_read(storage_level);
        if(file_descriptor == -1){
            file_descriptor = open_call();
        }
        return file_descriptor;
    }

    //Tries to close level and source_level
    static inline int client_submit_close(Info* i, int level, const std::function<int(int, int)>& close_call){
        auto* shareable_fd_state = static_cast<ShareableFileDescriptorsState*>(i->file_descriptor_state);
        auto& source_descriptors_info = shareable_fd_state->get_descriptors_info_lsr();
        auto& descriptors_info = shareable_fd_state->get_descriptors_info(level);
        int res = -1;
        {
            std::lock_guard<std::mutex> lock(*shareable_fd_state->get_mutex());
            //Means that we also need to close the source, since storage changed in mid read.
            if(shareable_fd_state->storage_changed(level)) {
                //Check if fd wasn't closed already by a concurrent thread.
                if(std::get<0>(source_descriptors_info) != -1) {
                    int &n_readers = std::get<1>(source_descriptors_info);
                    //If the storage changed then it means that the client did not signal an open (background threads left the fd open),
                    //hence we don't decrease the reader's counter for the staging level.
                    //We only need to check if there is any other client thread reading or that started reading from the source level of
                    //this file (that also suffered from a storage change mid read)
                    //If no one is reading from the "old" level we can close it.
                    if (n_readers <= 0 || --n_readers == 0) {
                        //If no new thread is already reading from the staging level we close it.
                        if(std::get<1>(descriptors_info) == 0) {
                            close_call(std::get<0>(descriptors_info), level);
                            std::get<0>(descriptors_info) = -1;
                        }
                        //Close source level
                        res = close_call(std::get<0>(source_descriptors_info), shareable_fd_state->get_last_storage_read());
                        std::get<0>(source_descriptors_info) = -1;
                        shareable_fd_state->update_last_storage_read(level);
                    }
                }
            }else{
                auto status = i->placed_state->get_placement_status();
                //This means that the placement was slower than the read. We decrease the counter and check if no one is reading. We also check if the background thread still needs the fd open.
                if(--std::get<1>(descriptors_info) == 0 && (status == PlacementStatusType::NOT_ELECTED || status == PlacementStatusType::IN_PLACE)){
                    res = close_call(std::get<0>(descriptors_info), level);
                    std::get<0>(descriptors_info) = -1;
                }
            }
        }
        return res;
    }

    static inline int background_submit_open(Info* i, int storage_level, const std::function<int()>& open_call){
        //background threads are opening a new file. Open needs to go through!!
        //No locking is needed here, since no one is accessing this file, except for the caller thread.
        if(i->storage_level != storage_level){
            auto& descriptors_info = static_cast<ShareableFileDescriptorsState*>(i->file_descriptor_state)->get_descriptors_info(storage_level);
            std::get<1>(descriptors_info)++;
            std::get<0>(descriptors_info) = open_call();
        }
        //This is viable for the source level, since we are protected by the placed_state info
        return static_cast<ShareableFileDescriptorsState*>(i->file_descriptor_state)->get_file_descriptor(storage_level);
    }

    static inline int background_submit_close(Info* i, int target_level, int source_level, const std::function<int(int, int)>& close_call) {
        auto* shareable_fd_state = static_cast<ShareableFileDescriptorsState*>(i->file_descriptor_state);
        auto& descriptors_info = shareable_fd_state->get_descriptors_info(target_level);
        std::lock_guard<std::mutex> lock(*shareable_fd_state->get_mutex());
        //We don't decrease the counter when target is source level, since we don't increase it for the background open.
        //Checking the n_readers on the "next"/"source" level guarantees that the file descriptor for the staging level
        //is only closed when no one is reading from the source level and this level simultaneously.
        //Used to counter the race condition of having the same file being read on different epochs by different threads.
        if((target_level == source_level && std::get<1>(descriptors_info) == 0)
            || (target_level != source_level && --std::get<1>(descriptors_info) + std::get<1>(shareable_fd_state->get_descriptors_info(source_level)) == 0)){
            int res = close_call(std::get<0>(descriptors_info), target_level);
            std::get<0>(descriptors_info) = -1;
            return res;
        }
        return -1;
    }
};

#endif //MONARCH_SHAREABLE_FILE_DESCRIPTORS_MANAGER_H
