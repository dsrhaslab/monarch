//
// Created by dantas on 04/06/21.
//

#ifndef THESIS_LIST_TRANSFORMS_H
#define THESIS_LIST_TRANSFORMS_H

#include <vector>
#include <string>
#include <tuple>

class ListTransforms {
private:
    template<typename T>
    static void expand_list(int rank, int world_size, std::vector<T>& res, std::vector<T> &l, std::tuple<int, int> l_info);
    static std::vector<int> generate_ids_list(int start, int end);
    static std::vector<std::string> generate_filenames_list(std::vector<std::string>& filenames, int start, int end);
    template<typename T>
    static std::vector<T> expand_list(int rank, int world_size, std::vector<T>& l, int expand_size);
    template<typename T>
    static std::vector<T> expand_list(int rank, int world_size, std::vector<T>& l, const std::vector<int>& shuffling_seeds);

    template<typename T>
    static std::vector<T> concatenate_and_expand_list(int rank, int world_size, std::vector<T>& l1, std::vector<T>& l2, int expand_size);
    template<typename T>
    static std::vector<T> concatenate_and_expand_list(int rank, int world_size, std::vector<T>& l1, std::vector<T>& l2, const std::vector<int>& shuffling_seeds);
public:
    static std::tuple<int, int> get_sizes(int original_size, int world_size);
    static std::vector<int> make_list(int rank, int world_size, std::vector<std::tuple<std::string, int>>& dirs_file_count, int epochs);
    static std::vector<int> make_shuffled_list(int rank, int world_size, std::vector<std::tuple<std::string, int>>& dirs_file_count, const std::vector<int>& shuffling_seeds);
    static std::vector<std::string> make_list(std::vector<std::tuple<std::string, int>>& dirs_file_count, std::vector<std::string> filenames, int epochs);
    static std::vector<std::string> make_shuffled_list(std::vector<std::tuple<std::string, int>>& dirs_file_count, std::vector<std::string> filenames, const std::vector<int>& shuffling_seeds);
};

#endif //THESIS_LIST_TRANSFORMS_H
