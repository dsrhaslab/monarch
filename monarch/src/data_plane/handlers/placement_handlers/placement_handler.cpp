//
// Created by dantas on 02/06/21.
//

#include "placement_handler.h"

#include "../../root/hierarchical_data_plane.h"
#include "../../metadata/prefetched_file.h"

PlacementHandler::PlacementHandler(HierarchicalDataPlane* data_plane, PlacementPolicy placement_policy) : data_plane_(data_plane), placement_policy_(placement_policy) {}

Status PlacementHandler::targeted_placement(File* f, int target_level, bool with_alloc){
    //must be made in the calling thread
    if(with_alloc) {
        data_plane_->get_alloc_driver(target_level)->allocate_storage(f->get_info());
    }
    data_plane_->t_pool(target_level)->push([this, f, target_level, name = f->get_filename()](int id){
        auto* fi = f->get_info();
        if(!f->is_full_read()) {
            data_plane_->debug_write(
                    f->get_info()->get_name() + " is a partial request. Reshaping to full request for placement");
            f->reshape_to_full_request();
            if (fi->has_shareable_file_descriptors() && data_plane_->is_file_system(fi->get_storage_level())) {
                auto* fs_driver = data_plane_->get_fs_driver(fi->get_storage_level());
                fs_driver->open_descriptor(fi, false);
                data_plane_->read_from_storage(f, fi->get_storage_level());
                fs_driver->close_descriptor(fi);
            }else{
                data_plane_->read_from_storage(f, fi->get_storage_level());
            }
        }
        Status status = data_plane_->write(f, target_level);
        data_plane_->placed_samples++;
        if(!data_plane_->get_alloc_driver(target_level)->in_memory_type())
            delete f;

        if (status.state == SUCCESS) {
            data_plane_->debug_write(fi->get_name() + " placed on level: " + std::to_string(target_level));
            fi->loaded_to(target_level);
        }else{
            data_plane_->debug_write(fi->get_name() + " failed placement on level: " + std::to_string(target_level));
        }
    });

    return {SUCCESS};
}

Status PlacementHandler::push_down_placement(File *f) {
    // search for a storage level with empty space except the last level
    int level = data_plane_->alloc_free_level(f->get_info(), 0);
    if (level >= 0){
        data_plane_->debug_write(f->get_info()->get_name() + " came from source. Found storage space on level: " +
                    std::to_string(level));
        //alloc is made with alloc_free_level
        return targeted_placement(f, level, false);
    }
    else{
        //race conditions are irrelevant with reached_stability
        data_plane_->reached_stability = true;
        data_plane_->debug_write(f->get_info()->get_name() + " came from source. No storage space in any level for " +
                    f->get_info()->get_name());
        data_plane_->debug_write("Reached stability with push down placement!");
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

Status PlacementHandler::place(File* f){
    if(placement_policy_ == FIRST_LEVEL_ONLY){
        return targeted_placement(f, 0, true);
    }else{
        return push_down_placement(f);
    }
}

Status PlacementHandler::place(PrefetchedFile* pf){
    return place(static_cast<File*>(pf));
}