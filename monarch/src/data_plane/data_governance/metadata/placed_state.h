//
// Created by dantas on 29/04/21.
//

#ifndef MONARCH_PLACED_STATE_H
#define MONARCH_PLACED_STATE_H

#include <atomic>

#include "../../types.h"

class PlacedState {
    //TODO improve semantics
    PlacementStatusType placement_status;
    std::atomic<bool> placement_started;
    std::atomic<bool> placement_ended;

public:

    PlacedState() : placement_status(PlacementStatusType::NOT_ELECTED), placement_started(false), placement_ended(false){}

    inline bool start_placement(){
        bool expected = false;
        return placement_started.compare_exchange_strong(expected, true);
    }

    inline void end_placement(){
        placement_ended = true;
    }

    //TODO check if it can be optimized
    inline PlacementStatusType get_placement_status() {
        //We don't update the var placement_status beforehand and return it in all cases for race conditions reasons.
        if(placement_ended){
            return PlacementStatusType::IN_PLACE;
        }else if(placement_started){
            return PlacementStatusType::STARTED;
        }else{
            return placement_status;
        }
    };

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

#endif //MONARCH_PLACED_STATE_H
