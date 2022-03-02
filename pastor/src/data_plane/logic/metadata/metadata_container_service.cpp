//
// Created by dantas on 26/03/21.
//
#include "metadata_container_service.h"
#include "list_transforms.h"
#include "placed_file_info.h"
#include "strict_file_info.h"

#include "absl/strings/match.h"

#include <iostream>
#include <utility>
#include <fstream>
#include <random>
#include <dirent.h>
#include <sys/stat.h>

template <class T>
MetadataContainerService<T>::MetadataContainerService(){
    current_epoch = 0;
    partition_file_count = 0;
    world_size = 1;
    train_full_size = 0;
}

template <class T>
MetadataContainerService<T>::MetadataContainerService(const std::string& data_dir_, const std::string& type_)
    : MetadataContainerService()
{
    local_parse = true;
    data_dir = data_dir_;
    type = type_;
    dirs_file_count = {
            std::make_pair("train", 0),
            std::make_pair("val", 0)
    };
    train_full_size = 0;
}

template <class T>
T* MetadataContainerService<T>::get_metadata(const std::string& name){
    return name_to_info[name];
}

template <class T>
T* MetadataContainerService<T>::get_metadata(int id){
    return id_to_info[id];
}

template <class T>
T* MetadataContainerService<T>::get_metadata_from_fildes(int fildes){
    concurrent_map_tbb::const_accessor a;
    if(!file_descriptors_to_info.find(a, fildes)){
        return nullptr;
    }else{
        FileInfo* fi = a->second;
        // Release lock for this filename
        a.release();
        return fi;
    }
}


template <class T>
T* MetadataContainerService<T>::get_metadata_from_ordered_id(int rank, int id){
    return id_to_info[partitioned_samples_ordered_ids[rank][id]];
}

template <class T>
void MetadataContainerService<T>::store_fildes(int fildes, T* info){
    concurrent_map_tbb::accessor a;
    bool new_file = file_descriptors_to_info.insert(a, fildes);

    if (!new_file) {
        // Release lock for this filename
        a.release();
    } else {
        // Add file with no content to buffer
        a->second = info;

        // Release lock for this filename
        a.release();
    }
}

template <class T>
T* MetadataContainerService<T>::remove_fildes(int fildes){
    concurrent_map_tbb::const_accessor a;
    bool file_in_buffer = file_descriptors_to_info.find(a, fildes);
    if(file_in_buffer) {
        T* f = a->second;
        a.release();
        file_descriptors_to_info.erase(fildes);
        return f;
    }else{
        return nullptr;
    }
}

template <class T>
T* MetadataContainerService<T>::get_file(int index){
    return id_to_info[index];
}

template <class T>
int MetadataContainerService<T>::get_iter_size(){
    return partition_file_count * epochs;
}

template <class T>
int MetadataContainerService<T>::get_current_epoch(){
    return current_epoch;
}

template <class T>
size_t MetadataContainerService<T>::get_full_size(){
    return full_size;
}

template <class T>
int MetadataContainerService<T>::get_file_count(){
    return file_count;
}

template <class T>
size_t MetadataContainerService<T>::get_train_full_size(){
    return train_full_size;
}

template <class T>
void MetadataContainerService<T>::set_distributed_id(int id){
    distributed_id = id;
}

template <class T>
void MetadataContainerService<T>::set_world_size(int world_size_){
    world_size = world_size_;
}

template <class T>
void MetadataContainerService<T>::set_file_count(int fc){
    MetadataContainerService::file_count = fc;
    MetadataContainerService::partition_file_count = fc;
}

template <class T>
void MetadataContainerService<T>::set_full_size(size_t full_size_){
    MetadataContainerService::full_size = full_size_;
}

template <class T>
void MetadataContainerService<T>::set_epochs(int epochs_){
    MetadataContainerService::epochs = epochs_;
}

template <class T>
void MetadataContainerService<T>::set_storage_source_level(int source_level){
    storage_source_level = source_level;
}

template <class T>
void MetadataContainerService<T>::set_shareable_file_descriptors(bool value){
    has_shareable_file_descriptors = value;
}

template <class T>
void MetadataContainerService<T>::set_train_files_regex(std::string& regex){
    train_files_regex = regex;
}

template <class T>
void MetadataContainerService<T>::set_stores_ids(bool value){
    stores_ids = value;
}

template <class T>
void MetadataContainerService<T>::add_dir_file_count(const std::string& name, size_t count){
    dirs_file_count.emplace_back(name, count);
}

template <class T>
void MetadataContainerService<T>::add_target_class_to_id(const std::string& name, int id){
    target_class_to_id.insert(std::make_pair(name, id));
}

template <class T>
void MetadataContainerService<T>::generate_samples_ordered_ids(){
    if(epochs <= 0)
        std::cerr << "Instance: " << distributed_id << " generate samples ordered ids: No epochs provided\n";
    else {
        /*
        if (distributed_id == 0) {
            std::cout << "Instance: " << distributed_id << " received train_length: " << std::get<1>(dirs_file_count[0])
                      << std::endl;
            if(dirs_file_count.size() > 1)
                std::cout << "Instance: " << distributed_id << " received val_length: " << std::get<1>(dirs_file_count[1])
                          << std::endl;
        }*/
        // i = rank
        if(world_size > 1)
            for (int i = 0; i < world_size; i++)
                partitioned_samples_ordered_ids.push_back(ListTransforms::make_list(i, world_size, dirs_file_count, epochs));
        else
            partitioned_samples_ordered_ids.push_back(ListTransforms::make_list(-1, 1, dirs_file_count, epochs));
    }
}

template <class T>
void MetadataContainerService<T>::generate_samples_ordered_ids(const std::vector<int>& shuffling_seeds){
    if(epochs <= 0)
        std::cerr << "Instance: " << distributed_id << " generate samples ordered ids: No epochs provided\n";
    else{
        /*
        if(distributed_id == 0) {
            std::cout << "Instance: " << distributed_id << " received train_length: " << std::get<1>(dirs_file_count[0])
                      << std::endl;
            if(dirs_file_count.size() > 1)
                std::cout << "Instance: " << distributed_id << " received val_length: " << std::get<1>(dirs_file_count[1])
                          << std::endl;
        }*/
        // i = rank
        if(world_size > 1)
            for (int i = 0; i < world_size; i++)
                partitioned_samples_ordered_ids.push_back(ListTransforms::make_shuffled_list(i, world_size, dirs_file_count, shuffling_seeds));
        else
            partitioned_samples_ordered_ids.push_back(ListTransforms::make_shuffled_list(-1, 1, dirs_file_count, shuffling_seeds));
    }
}

template <class T>
void MetadataContainerService<T>::add_entry(int id, T* fi){
    name_to_info.insert(std::make_pair(fi->get_name(), fi));
    id_to_info.insert(std::make_pair(id, fi));
}

template <class T>
std::vector<std::string> MetadataContainerService<T>::get_dirs_for_environment(){
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

template <class T>
int MetadataContainerService<T>::get_id(int rank, int index){
    return partitioned_samples_ordered_ids[rank][index];
}

//Transforms file_count into partitioned size
template <class T>
void MetadataContainerService<T>::make_partition(int rank_, int world_size_, int worker_id_, int num_workers_){
    int train_size = std::get<1>(dirs_file_count[0]);
    auto train_partition_info = ListTransforms::get_sizes(train_size, world_size_);
    int partition_train_size = std::get<0>(train_partition_info);
    //update state with new training size
    dirs_file_count[0] = std::make_pair(std::get<0>(dirs_file_count[0]), partition_train_size);
    partition_file_count = partition_train_size;

    if(dirs_file_count.size() > 1){
        int val_size = std::get<1>(dirs_file_count[1]);
        auto val_partition_info = ListTransforms::get_sizes(val_size, world_size_);
        int partition_val_size = std::get<0>(val_partition_info);
        //update state with new val size
        dirs_file_count[1] = std::make_pair(std::get<0>(dirs_file_count[1]), partition_val_size);
        partition_file_count += partition_val_size;
    }
    /*
        //generates imbalaced load on the last rank
        file_count = std::ceil(size_per_world / num_workers_);
        if (size_per_world % num_workers_ != 0){
            std::cout << "Total number of samples per world is not evenly divisible by num_workers_. Droping last" <<std::endl;
            file_count = std::ceil((size_per_world - num_workers_) / num_workers_);
        }
    */
}

template <class T>
FileInfo* MetadataContainerService<T>::parse_file(const std::string& path, const std::string& filename, const std::string& type){
    struct stat sbuf;
    stat((path + "/" + filename).c_str(), &sbuf);
    FileInfo* fi;

    if (type == "root_standalone")
        fi = new PlacedFileInfo(filename, sbuf.st_size, storage_source_level, has_shareable_file_descriptors);
    else
        fi = new StrictFileInfo(filename, sbuf.st_size, storage_source_level, has_shareable_file_descriptors);

    if(stores_ids)
        id_to_info.insert(std::make_pair(file_count, fi));

    name_to_info.insert(std::make_pair(fi->get_name(), fi));
    file_count++;
    full_size += sbuf.st_size;
    if (absl::StartsWith(filename, train_files_regex)){
        train_full_size += sbuf.st_size;
    }
    return fi;
}

template <class T>
bool MetadataContainerService<T>::is_train_file(const std::string& filename){
    return train_files_regex.empty() || absl::StartsWith(filename, train_files_regex);
}


//TODO parallelize parse when many dirs are present
template <class T>
void MetadataContainerService<T>::execute_local_parse(){
    int current_target = -1;
    int target_count = 0;
    for(auto & d : dirs_file_count){
        int dir_file_count = 0;
        std::string objective_dir = std::get<0>(d);
        std::string complete_root_dir_str = data_dir + "/" + objective_dir;
        struct dirent *target_entry;
        DIR* objective_dir_open = opendir(complete_root_dir_str.c_str());
        if(objective_dir_open){
            //traverse targets of a dir
            while((target_entry = readdir(objective_dir_open)) != nullptr){
                if(!absl::StartsWith(target_entry->d_name, ".")) {
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
                    }else {
                        current_target = entry->second;
                    }
                    //traverse all samples from a target
                    struct dirent *file_entry;
                    DIR* file_open = opendir(complete_target_dir.c_str());
                    while((file_entry = readdir(file_open)) != nullptr) {
                        if (!absl::StartsWith(target_entry->d_name, ".")) {
                            std::string filename;
                            filename.append(objective_dir)
                                    .append("/")
                                    .append(target_name)
                                    .append("/")
                                    .append(file_entry->d_name);

                            auto *cfi = parse_file(data_dir, filename, type);
                            cfi->set_target(current_target);
                            dir_file_count++;
                        }
                    }
                }
            }
            d = std::make_pair(std::get<0>(d), dir_file_count);
        }else {
            DIR* root_dir_open = opendir(data_dir.c_str());
            struct dirent *root_entry;
            while((root_entry = readdir(root_dir_open)) != nullptr){
                if (!absl::StartsWith(root_entry->d_name, ".")) {
                    parse_file(data_dir, root_entry->d_name, type);
                }
            }
            dirs_file_count = {std::make_pair("root", file_count)};
            break;
        }
    }
    if(epochs > 0) {
        // Seed with a real random value, if available
        std::random_device r;
        // Choose a random mean between 1 and 6
        std::default_random_engine e1(r());
        std::uniform_int_distribution<int> uniform_dist(1, 1000);
        std::vector<int> shuffling_seeds;
        for (int i = 0; i < epochs; i++)
            shuffling_seeds.push_back(uniform_dist(e1));
        generate_samples_ordered_ids(shuffling_seeds);
    }
}

template <class T>
void MetadataContainerService<T>::init(){
    if(local_parse){
        execute_local_parse();
    }
    //TODO else <- bring remote logic to here if possible
}

/*
 * int MetadataContainerService::calculate_offset(int rank, int world_size, int local_index){
    if(dirs_file_count.size() == 1)
        return rank * file_count + local_index;

    int train_l = std::get<1>(dirs_file_count[0]);
    int val_l = std::get<1>(dirs_file_count[1]);
    int epoch = std::floor(local_index / partition_file_count);
    int train_offset = rank * train_l + epoch * file_count;

    int index = local_index - partition_file_count * epoch;
    int is_val = index > train_l - 1 ? 1 : 0;

    int val_offset = ((world_size - rank - 1) * train_l + rank * val_l + 1) * is_val;
    return train_offset + val_offset + index;
}
 */


template class MetadataContainerService<FileInfo>;
