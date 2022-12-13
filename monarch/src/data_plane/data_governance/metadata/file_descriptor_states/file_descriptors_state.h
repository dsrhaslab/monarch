//
// Created by dantas on 17-10-2022.
//

#ifndef MONARCH_FILE_DESCRIPTOR_STATE_H
#define MONARCH_FILE_DESCRIPTOR_STATE_H

#include "../../../types.h"

class FileDescriptorsState {
public:
    virtual int get_file_descriptor(int level) = 0;
    virtual void set_file_descriptor(int value, int level) = 0;
    virtual FileDescriptorsStateType get_type() = 0;
};

#endif //MONARCH_FILE_DESCRIPTOR_STATE_H
