//
// Created by dantas on 02/06/21.
//
#include "eviction_handler.h"

#include "../../root/hierarchical_data_plane.h"
#include "../../metadata/strict_file_info.h"
#include "../../metadata/prefetched_file.h"

EvictionHandler::EvictionHandler(HierarchicalDataPlane* data_plane) : data_plane_(data_plane) {}

void EvictionHandler::persist_changes(StrictFileInfo* sfi, int from, int target) {
    //TODO: careful Drawback with this is that the notification is delayed until the end of the write.
    data_plane_->get_alloc_driver(from)->free_storage(sfi);
}

void EvictionHandler::eviction(StrictFileInfo* sfi, int target){
    data_plane_->debug_write("Evicting file: " + sfi->get_name() + " to storage level: " + std::to_string(target));
    int current_level = sfi->get_storage_level();
    PrefetchedFile *f;
    if (data_plane_->get_alloc_driver(current_level)->in_memory_type()) {
        data_plane_->debug_write("Removing for copy " + sfi->get_name());
        File* file = data_plane_->remove_for_copy(sfi, current_level);
        f = new PrefetchedFile(file, -1);
    } else {
        f = new PrefetchedFile(sfi, -1);
        data_plane_->debug_write("Reading for data transfer " + sfi->get_name());
        data_plane_->get_fs_driver(sfi->get_storage_level())->open_descriptor(sfi, O_RDONLY, false, false);
        data_plane_->read_from_storage(false, f, current_level);
        if(sfi->has_shareable_file_descriptors() && data_plane_->is_file_system(sfi->get_storage_level())){
            data_plane_->get_fs_driver(sfi->get_storage_level())->close_descriptor(sfi, false);
        }
        data_plane_->debug_write("Removing entry from previous storage " + sfi->get_name());
        //TODO this remove propably doesn't use the file descriptor available
        data_plane_->remove(sfi, current_level);
    }
    data_plane_->debug_write("Writing file " + sfi->get_name() + " to new storage level " + std::to_string(target));
    data_plane_->write(f, target);
    data_plane_->placed_samples++;
    sfi->moved_to(target);
    if(!data_plane_->get_alloc_driver(target)->in_memory_type())
        delete f;
    persist_changes(sfi, current_level, target);
}

void EvictionHandler::eviction(StrictFileInfo* sfi){
    data_plane_->debug_write("No storage space in any level. Fully discarding file: " + sfi->get_name());
    int current_level = sfi->get_storage_level();
    data_plane_->remove(sfi, current_level);
    data_plane_->placed_samples++;
    //move to intermediate stage level (example : Disk)
    sfi->moved_to(sfi->get_staging_level());
    persist_changes(sfi, current_level, sfi->get_staging_level());
}

void EvictionHandler::evict(StrictFileInfo* sfi){
    data_plane_->t_pool(sfi->get_storage_level())->push([this, sfi](int id) {
        int fl = data_plane_->alloc_free_level(sfi, 1);
        if(fl > 0){
            eviction(sfi, fl);
        }
        else{
            data_plane_->reached_stability = true;
            eviction(sfi);
        }
    });
}
