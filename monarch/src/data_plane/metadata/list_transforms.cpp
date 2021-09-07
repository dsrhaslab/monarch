//
// Created by dantas on 04/06/21.
//

#include "list_transforms.h"

#include <cmath>
#include <random>
#include <algorithm>

std::vector<int> ListTransforms::generate_ids_list(int start, int end){
    std::vector<int> res;
    for(int i = start; i < end; i++)
        res.push_back(i);

    return res;
}

std::vector<std::string> ListTransforms::generate_filenames_list(std::vector<std::string>& filenames, int start, int end){
    std::vector<std::string> res;
    for(int i = start; i < end; i++)
        res.push_back(filenames[i]);

    return res;
}

std::tuple<int, int> ListTransforms::get_sizes(int original_size, int world_size){
    int partitioned_size = std::ceil(original_size / world_size);
    if(partitioned_size % world_size != 0)
        partitioned_size = std::ceil((original_size - world_size) / world_size);
    int drop = original_size - partitioned_size * world_size;
    return {partitioned_size, drop};
}

template <typename T>
void ListTransforms::expand_list(int rank, int world_size_, std::vector<T>& res, std::vector<T> &l, std::tuple<int, int> l_info){
    int l_p_size = std::get<0>(l_info);
    int drop = std::get<1>(l_info);
    auto l_end = (rank < world_size_ - 1 && rank > -1) ? l.begin() + (l_p_size * (rank + 1)) : l.end() - drop;
    int real_rank = (rank > -1) ? rank : 0;
    res.insert(res.end(), l.begin() + (l_p_size * real_rank), l_end);
}

template<typename T>
std::vector<T> ListTransforms::concatenate_and_expand_list(int rank, int world_size,
                                                           std::vector<T>& l1, std::vector<T>& l2,
                                                           int expand_size){
    auto train_info = get_sizes(l1.size(), world_size);
    auto val_info = get_sizes(l2.size(), world_size);
    std::vector<T> res;

    for(int i = 0; i < expand_size; i++) {
        expand_list(rank, world_size, res, l1, train_info);
        expand_list(rank, world_size, res, l2, val_info);
    }
    return res;
}

template<typename T>
std::vector<T> ListTransforms::concatenate_and_expand_list(int rank, int world_size,
                                                           std::vector<T>& l1, std::vector<T>& l2,
                                                           const std::vector<int>& shuffling_seeds){

    auto train_info = get_sizes(l1.size(), world_size);
    auto val_info = get_sizes(l2.size(), world_size);
    std::vector<T> res;

    for(auto seed : shuffling_seeds) {
        std::shuffle(l1.begin(), l1.end() - 1, std::default_random_engine(seed));
        std::shuffle(l2.begin(), l2.end() - 1, std::default_random_engine(seed));

        expand_list(rank, world_size, res, l1, train_info);
        expand_list(rank, world_size, res, l2, val_info);
    }

    return res;
}

template<typename T>
std::vector<T> ListTransforms::expand_list(int rank, int world_size, std::vector<T>& l, int expand_size){
    auto train_info = get_sizes(l.size(), world_size);
    std::vector<T> res;

    for(int i = 0; i < expand_size; i++) {
        expand_list(rank, world_size, res, l, train_info);
    }
    return res;
}
template<typename T>
std::vector<T> ListTransforms::expand_list(int rank, int world_size, std::vector<T>& l, const std::vector<int>& shuffling_seeds){
    auto train_info = get_sizes(l.size(), world_size);
    std::vector<T> res;

    for(auto seed : shuffling_seeds) {
        std::shuffle(l.begin(), l.end() - 1, std::default_random_engine(seed));
        expand_list(rank, world_size, res, l, train_info);
    }

    return res;
}



std::vector<int> ListTransforms::make_list(int rank,
                                           int world_size,
                                           std::vector<std::tuple<std::string, int>>& dirs_file_count,
                                           int epochs){
    std::vector<int> train;
    std::vector<int> val;

    int train_l = std::get<1>(dirs_file_count[0]);
    int total = train_l;
    train = generate_ids_list(0, train_l);
    if(dirs_file_count.size() > 1){
        total += std::get<1>(dirs_file_count[1]);
        val = generate_ids_list(train_l, total);
        return concatenate_and_expand_list<int>(rank, world_size, train, val, epochs);
    }
    return expand_list<int>(rank, world_size, train, epochs);
}

std::vector<int> ListTransforms::make_shuffled_list(int rank, int world_size,
                                                    std::vector<std::tuple<std::string,
                                                    int>>& dirs_file_count,
                                                    const std::vector<int>& shuffling_seeds){
    std::vector<int> train;
    std::vector<int> val;

    int train_l = std::get<1>(dirs_file_count[0]);
    int total = train_l;
    train = generate_ids_list(0, train_l);
    if(dirs_file_count.size() > 1){
        int val_l = std::get<1>(dirs_file_count[1]);
        total += val_l;
        val = generate_ids_list(train_l, total);
        return concatenate_and_expand_list<int>(rank, world_size, train, val, shuffling_seeds);
    }
    return expand_list<int>(rank, world_size, train, shuffling_seeds);
}

std::vector<std::string> ListTransforms::make_list(std::vector<std::tuple<std::string, int>>& dirs_file_count,
                                                   std::vector<std::string> filenames, int epochs){
    std::vector<std::string> train;
    std::vector<std::string> val;

    int train_l = std::get<1>(dirs_file_count[0]);
    int total = train_l;
    train = generate_filenames_list(filenames, 0, train_l);
    if(dirs_file_count.size() > 1){
        int val_l = std::get<1>(dirs_file_count[1]);
        total += val_l;
        val = generate_filenames_list(filenames, train_l, total);
        return concatenate_and_expand_list<std::string>(-1, 1, train, val, epochs);
    }
    return expand_list<std::string>(-1, 1, train, epochs);
}

// Assumes root
std::vector<std::string> ListTransforms::make_shuffled_list(std::vector<std::tuple<std::string, int>>& dirs_file_count,
                                                            std::vector<std::string> filenames,
                                                            const std::vector<int>& shuffling_seeds){
    std::vector<std::string> train;
    std::vector<std::string> val;

    int train_l = std::get<1>(dirs_file_count[0]);
    int total = train_l;
    train = generate_filenames_list(filenames, 0, train_l);
    if(dirs_file_count.size() > 1){
        int val_l = std::get<1>(dirs_file_count[1]);
        total += val_l;
        val = generate_filenames_list(filenames, train_l, total);
        return concatenate_and_expand_list<std::string>(-1, 1, train, val, shuffling_seeds);
    }
    return expand_list<std::string>(-1, 1, train, shuffling_seeds);
}



