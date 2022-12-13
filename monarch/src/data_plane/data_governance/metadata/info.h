//
// Created by dantas on 17-06-2022.
//

#ifndef MONARCH_INFO_H
#define MONARCH_INFO_H

#include <string>

#include "../../types.h"
#include "../metadata/file_descriptor_states/single_file_descriptors_state.h"
#include "../metadata/file_descriptor_states/shareable_file_descriptors_state.h"
#include "placed_state.h"
#include "prefetched_state.h"

class Info{
    void build_states(const MetadataType& metadata_type_){
        if(has_shareable_file_descriptors(metadata_type_)){
            file_descriptor_state = new ShareableFileDescriptorsState(storage_level);
        }else{
            file_descriptor_state = new SingleFileDescriptorsState();
        }
        //if(has_strict_state(metadata_type_)){
        //    strict_state = new StrictState();
        //}
        if(has_placement_state(metadata_type_)){
            placed_state = new PlacedState();
        }
    }

public:
    Info() = delete;

    Info(const std::string& n, ssize_t s, int sl, const MetadataType& metadata_type_)
    {
        name = n;
        size = s;
        storage_level = sl;
        build_states(metadata_type_);
    }
    ssize_t size;
    std::string name;
    int storage_level;
    PlacedState* placed_state;
    //StrictState* strict_state;
    FileDescriptorsState* file_descriptor_state;
    PrefetchedState* prefetch_state;

    static bool has_placement_state(const MetadataType& metadata_type_){
        return metadata_type_.metadata_control_type == MetadataControlType::PLACED;
    };

    static bool has_strict_state(const MetadataType& metadata_type_){
        return metadata_type_.metadata_control_type == MetadataControlType::STRICT;
    };

    static bool has_shareable_file_descriptors(const MetadataType& metadata_type_){
        return CHK_OPTION(metadata_type_.optimization_options, METADATA_OPTION_SHAREABLE_FILE_DESCRIPTORS);
    };

    static bool has_data_offset_state(const MetadataType& metadata_type_){
        return CHK_OPTION(metadata_type_.optimization_options, METADATA_OPTION_DATA_OFFSET);
    };

    static bool has_prefetched_state(const MetadataType& metadata_type_){
        return CHK_OPTION(metadata_type_.optimization_options, METADATA_OPTION_PREFETCHED);
    };
};

#endif //MONARCH_INFO_H
