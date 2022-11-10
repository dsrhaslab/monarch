//
// Created by dantas on 19/09/22.
//

#include "metadata_container_service_builder.h"

MetadataContainerServiceBuilder::MetadataContainerServiceBuilder(const std::string& data_dir, int source_level){
    metadata_container_service = new MetadataContainerService(data_dir);
    metadata_container_service->storage_source_level = source_level;
}

MetadataContainerServiceBuilder::operator MetadataContainerService*() const {
    return metadata_container_service;
}

MetadataContainerService* MetadataContainerServiceBuilder::build() {
    return metadata_container_service;
}

MetadataContainerServiceBuilder& MetadataContainerServiceBuilder::with_local_parse(){
    metadata_container_service->local_parse = true;
}

MetadataContainerServiceBuilder& MetadataContainerServiceBuilder::with_remote_parse(){
    metadata_container_service->local_parse = false;
}

MetadataContainerServiceBuilder& MetadataContainerServiceBuilder::with_train_files_prefix(const std::string& prefix){
    metadata_container_service->train_files_prefix = prefix;
}

MetadataContainerServiceBuilder& MetadataContainerServiceBuilder::with_shared_file_descriptors(){
    SET_OPTION(metadata_container_service->metadata_type_.optimization_options, METADATA_OPTION_SHAREABLE_FILE_DESCRIPTORS);
}