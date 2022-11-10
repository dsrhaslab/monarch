//
// Created by dantas on 15/08/22.
//

#ifndef MONARCH_METADATA_CONTAINER_SERVICE_BUILDER_H
#define MONARCH_METADATA_CONTAINER_SERVICE_BUILDER_H

#include "metadata_container_service.h"

class MetadataContainerServiceBuilder {

    MetadataContainerService* metadata_container_service;

public:
    MetadataContainerServiceBuilder(const std::string& data_dir, int source_level);

    explicit operator MetadataContainerService*() const;

    MetadataContainerService* build();

    MetadataContainerServiceBuilder& with_local_parse();

    MetadataContainerServiceBuilder& with_remote_parse();

    MetadataContainerServiceBuilder& with_train_files_prefix(const std::string& prefix);

    MetadataContainerServiceBuilder& with_shared_file_descriptors();
};

#endif // MONARCH_METADATA_CONTAINER_SERVICE_BUILDER_H