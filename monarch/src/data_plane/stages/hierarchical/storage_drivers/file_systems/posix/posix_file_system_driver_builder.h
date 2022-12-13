//
// Created by dantas on 16/09/22.
//

#ifndef MONARCH_POSIX_FILE_SYSTEM_DRIVER_BUILDER_H
#define MONARCH_POSIX_FILE_SYSTEM_DRIVER_BUILDER_H

#include "../file_system_driver_builder.h"
#include "posix_file_system_driver.h"

class PosixFileSystemDriverBuilder : public FileSystemDriverBuilder {
public:
    explicit PosixFileSystemDriverBuilder(){
        base_storage_driver = new PosixFileSystemDriver();
    };

    PosixFileSystemDriverBuilder& with_max_block_size(){
        static_cast<PosixFileSystemDriver*>(base_storage_driver)->block_size = INT32_MAX;
        return *this;
    }

    PosixFileSystemDriverBuilder& with_block_size(size_t block_size){
        static_cast<PosixFileSystemDriver*>(base_storage_driver)->block_size = block_size;
        return *this;
    }
};

#endif //MONARCH_POSIX_FILE_SYSTEM_DRIVER_BUILDER_H