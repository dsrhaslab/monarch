//
// Created by dantas on 14/02/21.
//

#include <string>
#include <regex>
#include <iostream>
#include <fstream>
#include <random>
#include <cmath>
#include <dirent.h>

#include "metadata_container.h"
#include "../../data_plane/logic/metadata/strict_file_info.h"

MetadataContainer::MetadataContainer(const std::string& data_dir, const std::string& regex, int hierarchy_size, int epochs) {
    MetadataContainer::data_dir = data_dir;
    MetadataContainer::regex = regex;
    MetadataContainer::shuffling_enabled = false;
    MetadataContainer::file_count = 0;
    MetadataContainer::target_count = 0;
    MetadataContainer::storage_source_level = hierarchy_size - 1;
    MetadataContainer::epochs = epochs;
    MetadataContainer::dirs_file_count = {
            std::make_pair("train", 0),
            std::make_pair("val", 0)
    };
    MetadataContainer::has_subdirs = true;
    MetadataContainer::full_size = 0;
}


int MetadataContainer::create_target_entry(const std::string& target){
    auto entry = target_class_to_id.find(target);
    if(entry == target_class_to_id.end()) {
        target_class_to_id[target] = target_count;
        return target_count++;
    }
    return entry->second;
}

ControllerFileInfo* MetadataContainer::parse_file(const std::string& path, const std::string& filename){
    std::ifstream in_file(path + "/" + filename, std::ios::binary);
    in_file.seekg(0, std::ios::end);
    auto size = in_file.tellg();
    auto* cfi = new ControllerFileInfo();
    cfi->set_id(file_count);
    cfi->set_name(filename);
    cfi->set_size(size);
    infos.push_back(cfi);
    file_count++;
    full_size += size;
    return cfi;
}

void MetadataContainer::parse(){
    //traverse /train and /val dirs
    for(auto & d : dirs_file_count){
        int dir_file_count = 0;
        std::string objective_dir = std::get<0>(d);
        std::string complete_root_dir_str = data_dir + "/" + objective_dir;
        struct dirent *target_entry;
        DIR* objective_dir_open = opendir(complete_root_dir_str.c_str());
        if(objective_dir_open){
            //traverse targets of a dir
            while((target_entry = readdir(objective_dir_open)) != nullptr){
                if(!regex_match(std::string(target_entry->d_name), std::regex("[\.]+"))) {
                    std::string target_name = target_entry->d_name;
                    std::string complete_target_dir;
                    complete_target_dir.append(complete_root_dir_str)
                            .append("/")
                            .append(target_name);
                    //create and/or get identifier for this target
                    int current_target = create_target_entry(target_name);
                    //traverse all samples from a target
                    struct dirent *file_entry;
                    DIR* file_open = opendir(complete_target_dir.c_str());
                    std::cout << "Parsing " << complete_target_dir << std::endl;
                    while((file_entry = readdir(file_open)) != nullptr) {
                        if (regex_match(file_entry->d_name, std::regex(regex))) {
                            std::string filename;
                            filename.append(objective_dir)
                                    .append("/")
                                    .append(target_name)
                                    .append("/")
                                    .append(file_entry->d_name);

                            auto *cfi = parse_file(data_dir, filename);
                            cfi->set_target(current_target);
                            dir_file_count++;
                        }
                    }
                }
            }
            d = std::make_pair(std::get<0>(d), dir_file_count);
        }else {
            std::cout << "Train dir not found. Parsing " << data_dir << std::endl;
            DIR* root_dir_open = opendir(data_dir.c_str());
            struct dirent *root_entry;
            while((root_entry = readdir(root_dir_open)) != nullptr){
                if (regex_match(root_entry->d_name, std::regex(regex))) {
                    std::cout << "Parsing file: " << root_entry->d_name << std::endl;
                    parse_file(data_dir, root_entry->d_name);
                }
            }
            has_subdirs = false;
            break;
        }
    }
    if(has_subdirs)
        for(auto & d : dirs_file_count)
            std::cout << std::get<0>(d) << " count: " << std::get<1>(d) << std::endl;
    else
        std::cout << "root dir count: " << file_count << std::endl;
    // Seed with a real random value, if available
    std::random_device r;
    // Choose a random mean between 1 and 6
    std::default_random_engine e1(r());
    std::uniform_int_distribution<int> uniform_dist(1, 1000);
    for(int i = 0; i < epochs; i++)
        shuffling_seeds.push_back(uniform_dist(e1));
}

std::vector<std::tuple<std::string, long>> MetadataContainer::get_data_source_infos(){
    if(!has_subdirs)
        return {std::make_pair("root", file_count)};
    else
        return dirs_file_count;
}

std::vector<std::string> MetadataContainer::configs(){
    std::vector<std::string> res;
    res.push_back("- MetadataContainer\n");
    res.push_back("\tdata_dir: " + data_dir + "\n");
    res.push_back("\tregex: " + regex + "\n");
    res.push_back("\tfile_count: " + std::to_string(file_count) + "\n");
    res.push_back("\tfull_size: " + std::to_string(full_size) + "\n");
    if(shuffling_enabled){
        res.push_back("\tepochs: " + std::to_string(epochs) + "\n");
        res.push_back("\tavailable_dirs:\n");
        for(const auto& d : dirs_file_count)
            res.push_back("\t\t* " + std::get<0>(d) + " has " + std::to_string(std::get<1>(d)) + " files\n");
    }
    return res;
}
