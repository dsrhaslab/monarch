//
// Created by dantas on 26/03/21.
//

#include <utility>
#include <dirent.h>
#include <sys/stat.h>

#include "absl/strings/match.h"

#include "metadata_container_service.h"
#include "metadata_container_service_builder.h"
#include "list_transforms.h"


MetadataContainerServiceBuilder MetadataContainerService::create(const std::string& data_dir, int source_level){
    return MetadataContainerServiceBuilder{data_dir, source_level};
}

MetadataContainerService::MetadataContainerService(const std::string& data_dir){
    file_count = 0;
    full_size = 0;
    train_full_size = 0;
    local_parse = true;
    data_dir_ = data_dir;
    dirs_file_count = {
            std::make_pair("train", 0),
            std::make_pair("val", 0)
    };
}

Info* MetadataContainerService::get_metadata(const std::string& name){
    return name_to_info[name];
}


size_t MetadataContainerService::get_full_size(){
    return full_size;
}

int MetadataContainerService::get_file_count(){
    return file_count;
}

size_t MetadataContainerService::get_train_full_size(){
    return train_full_size;
}


void MetadataContainerService::set_file_count(int fc){
    MetadataContainerService::file_count = fc;
}


void MetadataContainerService::set_full_size(size_t full_size_){
    MetadataContainerService::full_size = full_size_;
}

void MetadataContainerService::add_dir_file_count(const std::string& name, size_t count){
    dirs_file_count.emplace_back(name, count);
}


void MetadataContainerService::add_entry(Info* fi){
    name_to_info.insert(std::make_pair(fi->name, fi));
}


std::vector<std::string> MetadataContainerService::get_dirs_for_environment(){
    std::vector<std::string> res;
    if(dirs_file_count.size() > 1) {
        for (auto entry1 : dirs_file_count) {
            for (auto &entry2 : target_class_to_id) {
                res.push_back(std::get<0>(entry1) + "/" + entry2.first);
            }
        }
    }
    return res;
}


Info* MetadataContainerService::parse_file(const std::string& path, const std::string& filename){
    struct stat sbuf;
    stat((path + "/" + filename).c_str(), &sbuf);
    Info* fi = new Info(filename, sbuf.st_size, storage_source_level, metadata_type_);
    name_to_info.insert(std::make_pair(filename, fi));
    file_count++;
    full_size += sbuf.st_size;
    if (is_train_file(filename)){
        train_full_size += sbuf.st_size;
    }
    return fi;
}


bool MetadataContainerService::is_train_file(const std::string& filename){
    return train_files_prefix.empty() || absl::StartsWith(filename, train_files_prefix);
}


//TODO parallelize parse when many dirs are present

void MetadataContainerService::execute_local_parse(){
    int current_target = -1;
    int target_count = 0;
    for(auto & d : dirs_file_count) {
        int dir_file_count = 0;
        std::string objective_dir = std::get<0>(d);
        std::string complete_root_dir_str = data_dir_ + "/" + objective_dir;
        struct dirent *target_entry;
        DIR *objective_dir_open = opendir(complete_root_dir_str.c_str());
        if (objective_dir_open) {
            //traverse targets of a dir
            while ((target_entry = readdir(objective_dir_open)) != nullptr) {
                if (!absl::StartsWith(target_entry->d_name, ".")) {
                    std::string target_name = target_entry->d_name;
                    std::string complete_target_dir;
                    complete_target_dir.append(complete_root_dir_str)
                            .append("/")
                            .append(target_name);
                    //create and/or get identifier for this target
                    auto entry = target_class_to_id.find(target_name);
                    if (entry == target_class_to_id.end()) {
                        target_class_to_id[target_name] = target_count;
                        current_target = target_count++;
                    } else {
                        current_target = entry->second;
                    }
                    //traverse all samples from a target
                    struct dirent *file_entry;
                    DIR *file_open = opendir(complete_target_dir.c_str());
                    while ((file_entry = readdir(file_open)) != nullptr) {
                        if (!absl::StartsWith(target_entry->d_name, ".")) {
                            std::string filename;
                            filename.append(objective_dir)
                                    .append("/")
                                    .append(target_name)
                                    .append("/")
                                    .append(file_entry->d_name);

                            parse_file(data_dir_, filename);
                            // TODO how to include the target for the intrusive solution in the current arch.
                            // cfi->set_target(current_target);
                            dir_file_count++;
                        }
                    }
                }
            }
            d = std::make_pair(std::get<0>(d), dir_file_count);
        } else {
            DIR *root_dir_open = opendir(data_dir_.c_str());
            struct dirent *root_entry;
            while ((root_entry = readdir(root_dir_open)) != nullptr) {
                if (!absl::StartsWith(root_entry->d_name, ".")) {
                    parse_file(data_dir_, root_entry->d_name);
                }
            }
            dirs_file_count = {std::make_pair("root", file_count)};
            break;
        }
    }
}

void MetadataContainerService::init(){
    if(local_parse){
        execute_local_parse();
    }
    //TODO else <- bring remote logic to here if possible
}