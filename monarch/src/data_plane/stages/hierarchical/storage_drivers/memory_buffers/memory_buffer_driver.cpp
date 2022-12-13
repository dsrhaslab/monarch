//
// Created by dantas on 18-10-2022.
//

#include "memory_buffer_driver.h"
#include "memory_buffer_driver_builder.h"

StorageDriverType MemoryBufferDriver::get_type() {
    return StorageDriverType::MEMORY_BUFFER;
}

std::vector<std::string> MemoryBufferDriver::configs(){
    //TODO
}

int MemoryBufferDriver::generate_file_descriptor(){
    return memory_fd_value_--;
}

MemoryBufferDriverBuilder* MemoryBufferDriver::create(StorageDriverSubType subtype){
    return new MemoryBufferDriverBuilder(subtype);
}