//
// Created by dantas on 19/10/221.
//

#ifndef MONARCH_STORAGE_DRIVER_STATE_H
#define MONARCH_STORAGE_DRIVER_STATE_H

#include <cstdio>
#include <vector>
#include <string>

#include "../../../../data_governance/metadata/info.h"
#include "../../../../utils/status.h"
#include "../../../../types.h"

class StorageDriverState
{
public:
    virtual Status<ssize_t> allocate_storage(Info* i) = 0;

    virtual Status<ssize_t> free_storage(Info* i) = 0;

    virtual bool becomes_full(Info* i) = 0;

    virtual size_t resize(size_t new_size) = 0;

    virtual size_t get_current_storage_size() = 0;

    virtual size_t get_max_storage_size() const = 0;

    virtual StorageDriverStateType get_type() = 0;

    virtual std::vector<std::string> configs() = 0;
};

#endif // MONARCH_STORAGE_DRIVER_STATE_H