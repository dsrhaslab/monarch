//
// Created by dantas on 02/06/21.
//

#include <fcntl.h>

#include "placement_handler.h"

#include "../../root/hierarchical_data_plane.h"
#include "../../metadata/prefetched_file.h"

PlacementHandler::PlacementHandler(HierarchicalDataPlane* data_plane, PlacementPolicy placement_policy, bool async_placement)
    : data_plane_(data_plane), placement_policy_(placement_policy), async_placement_(async_placement){}

Status<ssize_t> PlacementHandler::target_placement_logic(File* f, int target_level){
    auto* fi = f->get_info();
    bool client_ended_read = false;
    if(!f->is_full_read()) {
        if(data_plane_->debug_logger->is_activated()){
            data_plane_->debug_write(
                    f->get_info()->get_name() + " is a partial request. Reshaping to full request for placement");
        }
        f->reshape_to_full_request();
        int source_level = fi->get_storage_level();
        if (fi->has_shareable_file_descriptors() && data_plane_->is_file_system(source_level)) {
            data_plane_->read_from_storage(false, f, source_level);
            //If close call returns 1 then client has already read the file and called close (without success)
            client_ended_read = data_plane_->get_fs_driver(source_level)->close_descriptor(fi, false) == 1;
            if (client_ended_read && data_plane_->profiler) {
                data_plane_->profiler->submit_background_metadata_request("close", source_level);
            }
        }else{
            data_plane_->read_from_storage(false, f, fi->get_storage_level());
        }
    }

    Status<ssize_t> status = NOT_FOUND;
    if (fi->has_shareable_file_descriptors() && data_plane_->is_file_system(target_level)) {
        auto* fs_driver = data_plane_->get_fs_driver(target_level);
        fs_driver->open_descriptor(fi, O_RDWR | O_CREAT, 0644, false, false);
        status = data_plane_->write(f, target_level);

        if(data_plane_->profiler){
            data_plane_->profiler->submit_background_metadata_request("open", target_level);
        }

        //Leave it open for mmap or when client is still going
        if(async_placement_){
            bool can_close = fs_driver->upper_level_conditional_close_descriptor(fi);
            if(data_plane_->profiler && can_close){
                data_plane_->profiler->submit_background_metadata_request("close", target_level);
            }
        }
    }
    else{
        status = data_plane_->write(f, target_level);
    }

    data_plane_->placed_samples++;
    if(!data_plane_->get_alloc_driver(target_level)->in_memory_type())
        delete f;

    if (status.state == SUCCESS) {
        if(data_plane_->debug_logger->is_activated()){
            data_plane_->debug_write(fi->get_name() + " placed on level: " + std::to_string(target_level));
        }
        fi->loaded_to(target_level);
    }else{
        if(data_plane_->debug_logger->is_activated()){
            data_plane_->debug_write(fi->get_name() + " failed placement on level: " + std::to_string(target_level));
        }
    }
    return status;
}

Status<ssize_t> PlacementHandler::targeted_placement(File* f, int target_level, bool with_alloc){
    //must be made in the calling thread
    if(with_alloc) {
        data_plane_->get_alloc_driver(target_level)->allocate_storage(f->get_info());
    }
    if(async_placement_){
        data_plane_->t_pool(target_level)->push([this, f, target_level](int id){
            target_placement_logic(f, target_level);
        });
    }else{
        return target_placement_logic(f, target_level);
    }
    return {SUCCESS};
}

Status<ssize_t> PlacementHandler::push_down_placement(File *f) {
    // search for a storage level with empty space except the last level
    int level = data_plane_->alloc_free_level(f->get_info(), 0);
    if (level >= 0){
        if(data_plane_->debug_logger->is_activated()){
            data_plane_->debug_write(f->get_info()->get_name() + " came from source. Found storage space on level: " +
            std::to_string(level));
        }
        //alloc is made with alloc_free_level
        return targeted_placement(f, level, false);
    }
    else{
        //race conditions are irrelevant with reached_stability
        data_plane_->reached_stability = true;
        if(data_plane_->debug_logger->is_activated()){
            data_plane_->debug_write(f->get_info()->get_name() + " came from source. No storage space in any level for " +
                f->get_info()->get_name());
            data_plane_->debug_write("Reached stability with push down placement!");
        }
        delete f;
        return {SUCCESS};
    }
}

bool PlacementHandler::check_placement_validity(){
    return data_plane_->storage_hierarchy_size > 1 &&
       !data_plane_->reached_stability;
}

bool PlacementHandler::check_placement_validity(PlacedFileInfo* pfi){
    return check_placement_validity() &&
        pfi->get_storage_level() == data_plane_->storage_hierarchy_size - 1 &&
        pfi->begin_placement();
}

Status<ssize_t> PlacementHandler::place(File* f){
    Status<ssize_t> s = NOT_FOUND;
    if(data_plane_->profiler){
        TimeSubmission ts;
        ts.set_start();
        s = base_placement(f);
        ts.set_end();
        data_plane_->profiler->submit_placement_time(ts);
    }else{
        s = base_placement(f);
    }
    return s;
}

Status<ssize_t> PlacementHandler::base_placement(File* f){
    if(placement_policy_ == FIRST_LEVEL_ONLY){
        return targeted_placement(f, 0, true);
    }else{
        return push_down_placement(f);
    }
}

Status<ssize_t> PlacementHandler::place(PrefetchedFile* pf){
    return place(static_cast<File*>(pf));
}
