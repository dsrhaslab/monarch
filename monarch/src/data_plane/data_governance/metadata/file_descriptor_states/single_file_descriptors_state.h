//
// Created by dantas on 17-10-2022.
//

#ifndef MONARCH_SINGLE_FILE_DESCRIPTOR_STATE_H
#define MONARCH_SINGLE_FILE_DESCRIPTOR_STATE_H

#include "file_descriptors_state.h"

class SingleFileDescriptorsState : public FileDescriptorsState {
    int file_descriptor = -1;

public:
    int get_file_descriptor(int level) override{
        return file_descriptor;
    };

    void set_file_descriptor(int value, int level) override{
        file_descriptor = value;
    };

    FileDescriptorsStateType get_type() override{
        return FileDescriptorsStateType::SINGLE;
    };
};

#endif //MONARCH_SINGLE_FILE_DESCRIPTOR_STATE_H
