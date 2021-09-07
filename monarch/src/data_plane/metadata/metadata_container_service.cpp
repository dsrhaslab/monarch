//
// Created by dantas on 26/03/21.
//
#include "metadata_container_service.h"
#include "list_transforms.h"

#include <iostream>
#include <utility>


template <class T>
MetadataContainerService<T>::MetadataContainerService(){
    current_epoch = 0;
    file_index = 0;
    partition_file_count = 0;
    world_size = 1;
    erroneous_state = false;
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
T* MetadataContainerService<T>::get_metadata_from_ordered_id(int rank, int id){
    return id_to_info[partitioned_samples_ordered_ids[rank][id]];
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
void MetadataContainerService<T>::set_storage_source_level(int level){
    MetadataContainerService::storage_source_level = level;
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
        if (distributed_id == 0) {
            std::cout << "Instance: " << distributed_id << " received train_length: " << std::get<1>(dirs_file_count[0])
                      << std::endl;
            if(dirs_file_count.size() > 1)
                std::cout << "Instance: " << distributed_id << " received val_length: " << std::get<1>(dirs_file_count[1])
                          << std::endl;
        }
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
        if(distributed_id == 0) {
            std::cout << "Instance: " << distributed_id << " received train_length: " << std::get<1>(dirs_file_count[0])
                      << std::endl;
            if(dirs_file_count.size() > 1)
                std::cout << "Instance: " << distributed_id << " received val_length: " << std::get<1>(dirs_file_count[1])
                          << std::endl;
        }
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
                res.push_back("/" + std::get<0>(entry1) + "/" + entry2.first);
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