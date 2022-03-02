//
// Created by dantas on 17/07/21.
//

#ifndef THESIS_LIBC_OPERATIONS_ENUMS_H
#define THESIS_LIBC_OPERATIONS_ENUMS_H

#if defined BAZEL_BUILD || defined TF_BAZEL_BUILD
#include "third_party/utils/enum.h"
#else
#include "enum.h"
#endif

/**
* OperationType class.
*/
BETTER_ENUM (OperationType,
    int,
    metadata_calls = 1,
    data_calls = 2);

/**
* Metadata Definitions.
*/
BETTER_ENUM (Metadata,
    int,
    no_op = 0,
    open = 1,
    open_variadic = 2,
    open64 = 3,
    open64_variadic = 4,
    creat = 5,
    creat64 = 6,
    openat = 7,
    openat_variadic = 8,
    close = 9);

/**
* Data Definitions.
*/
BETTER_ENUM (Data,
    int,
    no_op = 0,
    pread = 3,
    pwrite = 4,
    pread64 = 5,
    mmap=6);

#endif //THESIS_LIBC_OPERATIONS_ENUMS_H
