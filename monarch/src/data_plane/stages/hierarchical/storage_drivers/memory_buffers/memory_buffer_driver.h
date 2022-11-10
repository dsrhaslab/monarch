//
// Created by dantas on 26/09/22.
//

#ifndef MONARCH_MEMORY_BUFFER_DRIVER_H
#define MONARCH_MEMORY_BUFFER_DRIVER_H

#include <climits>

#include "../storage_driver.h"

class MemoryBufferDriverBuilder;

class MemoryBufferDriver : public StorageDriver
{
    std::atomic<int> memory_fd_value_{INT_MAX};

public:
    StorageDriverType get_type() override;

    std::vector<std::string> configs() override;

    int generate_file_descriptor();

    virtual File *remove_for_copy(Info *fi) = 0;

    static MemoryBufferDriverBuilder* create(StorageDriverSubType subtype);
};

#endif // MONARCH_MEMORY_BUFFER_DRIVER_H