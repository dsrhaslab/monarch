//
// Created by dantas on 02/06/21.
//

#include <fcntl.h>

#include "../../utils/logging/singleton_logger.h"
#include "placement_handler.h"

PlacementHandler::PlacementHandler(Monarch* monarch_stage, PlacementPolicy placement_policy, bool async_placement)
    : monarch_(monarch_stage), placement_policy_(placement_policy), async_placement_(async_placement){
        reached_stability_ = false;
        hierarchical_stage = monarch_->get_hierarchical_stage();
        profiler = ProfilerProxy::get_instance();
        debug_logger = SingletonLogger::get_instance();
    }

Status<ssize_t> PlacementHandler::target_placement_logic(File* f, int target_level){
    auto* i = f->info;
    int level = i->storage_level;
    if(!f->is_full_read) {
        if(debug_logger->is_activated()){
            debug_write(i->name 
                + " is a partial request. Reshaping to full request for placement");
        }
        f->reshape_to_full_request();
        //For now, it's always a file system
        monarch_->base_open(i, O_RDWR, {level, false, false});
        monarch_->read_from_storage(i, f->content, f->requested_size, f->offset, {level, false, false});
    }
    //Close source here even if it is a full read, otherwise the client won't close that fd
    monarch_->base_close(i, {level, false});
    monarch_->base_open(i, O_RDWR | O_CREAT, 0644, {target_level, false, false});
    Status<ssize_t> status = monarch_->write(f, target_level);
    monarch_->base_close(i, {target_level, false});
    if (status.state == SUCCESS) {
        if(debug_logger->is_activated()){
            debug_write(i->name 
                + " placed on level: " 
                + std::to_string(target_level));
        }
        //Set new storage level
        i->storage_level = target_level;
        i->placed_state->end_placement();
    }else{
        if(debug_logger->is_activated()){
            debug_write(i->name 
                + " failed placement on level: " 
                + std::to_string(target_level));
        }
    }
    return status;
}

Status<ssize_t> PlacementHandler::targeted_placement(File* f, int target_level, bool with_alloc){
    //must be made in the calling thread
    if(with_alloc) {
        hierarchical_stage->get_driver(target_level)->state->allocate_storage(f->info);
    }
    if(async_placement_){
        hierarchical_stage->t_pool(target_level)->push([this, f, target_level](int id){
            target_placement_logic(f, target_level);
            }
        );
    }else{
        return target_placement_logic(f, target_level);
    }
    return {SUCCESS};
}

Status<ssize_t> PlacementHandler::push_down_placement(File *f) {
    // search for a storage level with empty space except the last level
    int level = hierarchical_stage->alloc_free_level(f->info, 0);
    if (level >= 0){
        if(debug_logger->is_activated()){
            debug_write(f->info->name 
                + " came from source. Found storage space on level: " 
                + std::to_string(level));
        }
        //alloc is made with alloc_free_level
        return targeted_placement(f, level, false);
    }
    else{
        //race conditions are irrelevant with reached_stability
        reached_stability_ = true;
        if(debug_logger->is_activated()){
            debug_write(f->info->name 
                + " came from source. No storage space in any level for " 
                +f->info->name);
            debug_write("Reached stability with push down placement!");
        }
        delete f;
        return {SUCCESS};
    }
}

bool PlacementHandler::check_placement_validity(){
    return monarch_->get_hierarchical_stage()->has_staging_levels() &&
       !reached_stability_;
}

bool PlacementHandler::check_placement_validity(Info* i){
    return check_placement_validity() &&
        i->storage_level == monarch_->get_hierarchical_stage()->get_source_level() &&
        i->placed_state->start_placement();
}

Status<ssize_t> PlacementHandler::place(File* f){
    Status<ssize_t> s;
    if(profiler->is_activated()){
        TimeSubmission ts;
        ts.set_start();
        s = base_placement(f);
        ts.set_end();
        profiler->submit_placement_time(ts);
    }else{
        s = base_placement(f);
    }
    return s;
}

Status<ssize_t> PlacementHandler::base_placement(File* f){
    if(placement_policy_ == PlacementPolicy::FIRST_LEVEL_ONLY){
        return targeted_placement(f, 0, true);
    }else{
        return push_down_placement(f);
    }
}

void PlacementHandler::debug_write(const std::string& msg){
    debug_logger->_write("[PlacementHandler] " + msg);
}
