//
// Created by dantas on 26/09/22.
//

#ifndef MONARCH_FILE_SYSTEM_DRIVER_H
#define MONARCH_FILE_SYSTEM_DRIVER_H

#include <absl/strings/strip.h>
#include "../storage_driver.h"

class FileSystemDriverBuilder;
class FileSystemDriver : public StorageDriver {
protected:
    char* storage_prefix = nullptr;

    friend class FileSystemDriverBuilder;

public:
    StorageDriverType get_type() override {
        return StorageDriverType::FILE_SYSTEM;
    };

    std::vector<std::string> configs() override {
        //TODO
    };

    absl::string_view decode_filename(absl::string_view full_path){
        return absl::StripPrefix(full_path, storage_prefix);
    }

    const char* get_prefix(){
        return storage_prefix;
    }

    virtual void create_environment(std::vector<std::string> &dirs, bool enable_write) = 0;
};

#endif // MONARCH_FILE_SYSTEM_DRIVER_H