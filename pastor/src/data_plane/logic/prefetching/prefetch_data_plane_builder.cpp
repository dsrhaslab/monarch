//
// Created by dantas on 29/10/20.
//

#include "prefetch_data_plane_builder.h"
#include "../handlers/control_handler.h"
#include <iostream>

DataPlane* PrefetchDataPlaneBuilder::build(){
    data_plane->read_policy = PrefetchDataPlaneBuilder::read_policy;
    if(data_plane->prefetch_thread_pool == nullptr)
        data_plane->prefetch_thread_pool = new ctpl::thread_pool(2);
    switch(evict_call_type){
        case CLIENT:
            //TODO
            //data_plane->control_handler = new ControlHandler(data_plane, control_policy, placement_policy);
            break;
        case HOUSEKEEPER:
            //TODO
            break;
    }
    return data_plane;
}

PrefetchDataPlaneBuilder& PrefetchDataPlaneBuilder::configurations(){return *this;}

PrefetchDataPlaneBuilder& PrefetchDataPlaneBuilder::policies(){return *this;}

PrefetchDataPlaneBuilder &PrefetchDataPlaneBuilder::with_eviction_percentage(float percentage) {
    data_plane->eviction_percentage = percentage;
    return *this;
}

PrefetchDataPlaneBuilder &PrefetchDataPlaneBuilder::with_prefetch_thread_pool_size(int size) {
    data_plane->prefetch_thread_pool = new ctpl::thread_pool(size);
    return *this;
}


PrefetchDataPlaneBuilder& PrefetchDataPlaneBuilder::with_read_policy(ReadPolicy rp){
    read_policy = rp;
    data_plane->read_policy = rp;
    return *this;
}