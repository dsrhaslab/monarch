//
// Created by dantas on 19/10/20.
//

#ifndef MONARCH_STORAGE_DRIVER_H
#define MONARCH_STORAGE_DRIVER_H

#include <string>
#include <vector>

#include "../../../types.h"
#include "../../../data_governance/metadata/info.h"
#include "../../../data_governance/payload/file.h"
#include "../../../utils/status.h"
#include "states/storage_driver_state.h"

class StorageDriver
{
public:
    StorageDriverState* state;

    virtual Status<ssize_t> read(Info *fi, char *result, size_t n, off_t offset) = 0;

    virtual Status<ssize_t> write(File *f) = 0;

    virtual Status<ssize_t> remove(Info *fi) = 0;

    virtual StorageDriverType get_type() = 0;

    virtual StorageDriverSubType get_subtype() = 0;

    virtual std::vector<std::string> configs() = 0;
};

#endif // MONARCH_STORAGE_DRIVER_H