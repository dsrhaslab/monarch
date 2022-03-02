//
// Created by dantas on 04/11/20.
//

#include "data_storage_driver_builder.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"

DataStorageDriver* DataStorageDriverBuilder::build(){
    if (max_storage_size > 0){
        AllocableDataStorageDriver* alloc_driver;
        if(blocking_)
            alloc_driver = new BlockingAllocableDataStorageDriver(data_storage_driver);
        else
            alloc_driver = new AllocableDataStorageDriver(data_storage_driver);
        alloc_driver->max_storage_size = max_storage_size;
        alloc_driver->max_storage_occupation_threshold = threshold;
        return alloc_driver;
    }
    else
        return data_storage_driver;
}

DataStorageDriverBuilder& DataStorageDriverBuilder::with_hierarchy_level(int level){
    data_storage_driver->level = level;
    return *this;
}

DataStorageDriverBuilder& DataStorageDriverBuilder::with_block_size(size_t bs){
    data_storage_driver->block_size = bs;
    return *this;
}

DataStorageDriverBuilder& DataStorageDriverBuilder::with_storage_prefix(const std::string& prefix){
    if(!absl::EndsWith(prefix, "/")){
        data_storage_driver->storage_prefix =absl::StrCat(prefix, "/");
    }else{
        data_storage_driver->storage_prefix = prefix;
    }
    return *this;
}


DataStorageDriverBuilder& DataStorageDriverBuilder::with_allocation_capabilities(size_t mss, float t, bool blocking){
    max_storage_size = mss;
    threshold = t;
    blocking_ = blocking;
    return *this;
}

DataStorageDriverBuilder& DataStorageDriverBuilder::with_allocation_capabilities(size_t mss, bool blocking){
    max_storage_size = mss;
    blocking_ = blocking;
    return *this;
}
