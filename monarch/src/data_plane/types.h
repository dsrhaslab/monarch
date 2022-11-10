//
// Created by dantas on 03/06/21.
//

#ifndef MONARCH_TYPES_H
#define MONARCH_TYPES_H

#define METADATA_OPTION_SHAREABLE_FILE_DESCRIPTORS (1 << 0)
#define METADATA_OPTION_DATA_OFFSET                (1 << 1)
#define METADATA_OPTION_PREFETCHED                 (1 << 2)

#define SET_OPTION(n, f) ((n) |= (f))
#define CHK_OPTION(n, f) ((n) & (f))

#include "stdint.h"

/**
 * File Descriptor state type
 */

enum class FileDescriptorsStateType : unsigned int {
    SINGLE,
    SHAREABLE
};

/**
 * @brief Placement Status
 *
 */

enum class PlacementStatusType {
    NOT_ELECTED,
    ELECTED,
    STARTED,
    IN_PLACE
};

/**
 * @brief Handlers Policies
 * 
 */

enum class ControlPolicy {
    LOCK_ORDERED,
    QUEUE_ORDERED,
    SINGLE_THREAD_ORDERED,
    SOLO_PLACEMENT
};

enum class PlacementPolicy {
    FIRST_LEVEL_ONLY,
    PUSH_DOWN
};

enum class ReadPolicy {
    WAIT_ENFORCED,
    RELAXED
};

enum class EvictCallPolicy {
    CLIENT,
    HOUSEKEEPER
};

/**
 * @brief Data Governance types
 * 
 */

enum class MetadataControlType {
    NONE,
    PLACED,
    STRICT
};

struct MetadataType {
    MetadataControlType metadata_control_type = MetadataControlType::NONE;
    uint8_t optimization_options = 0b00000000;
};

/**
 * @brief Storage Drivers types
 * 
 */

enum class StorageDriverType {
    FILE_SYSTEM,
    MEMORY_BUFFER
};

enum class StorageDriverSubType {
    POSIX,
    THREAD_BUILDING_BLOCKS,
    PARALLEL_HASHMAP,
    PRE_ALLOC_MAP,
    PRE_ALLOC_MAP_WITH_SELECTION,
    PRE_ALLOC_ARRAY_WITH_SELECTION
};

enum class StorageDriverStateType {
    NONE,
    ALLOCABLE,
    BLOCKING,
    EVENTUAL,
};

#endif //MONARCH_TYPES_H
