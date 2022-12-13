//
// Created by dantas on 26/09/22.
//

#ifndef MONARCH_FILE_SYSTEM_DRIVER_BUILDER_H
#define MONARCH_FILE_SYSTEM_DRIVER_BUILDER_H

#include "absl/strings/match.h"

#include "file_system_driver.h"
#include "../storage_driver_builder.h"

class FileSystemDriverBuilder : public StorageDriverBuilder {
public:
    FileSystemDriverBuilder& with_storage_prefix(const std::string& storage_prefix){
        char* driver_storage_prefix;
        if(!absl::EndsWith(storage_prefix, "/")){
            driver_storage_prefix = new char[storage_prefix.length() + 1];
            strcpy( driver_storage_prefix, (storage_prefix + "/").c_str());
        }else{
            driver_storage_prefix = new char[storage_prefix.length()];
            strcpy(driver_storage_prefix, storage_prefix.c_str());
        }
        static_cast<FileSystemDriver*>(base_storage_driver)->storage_prefix = driver_storage_prefix;
        return *this;
    }
};

#endif // MONARCH_FILE_SYSTEM_DRIVER_BUILDER_H