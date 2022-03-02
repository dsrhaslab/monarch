//
// Created by dantas on 08/08/21.
//
#include <iostream>
#include <dirent.h>
#include <regex>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

std::vector<std::string> get_filenames(const std::string& data_path, bool provide_full_path){
    std::vector<std::string> res;
    for(auto& objective_dir : {"train", "val"}){
        std::string complete_root_dir_str = data_path + "/" + objective_dir;
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
                    //traverse all samples from a target
                    struct dirent *file_entry;
                    DIR* file_open = opendir(complete_target_dir.c_str());
                    while((file_entry = readdir(file_open)) != nullptr) {
                        std::string filename;
                        if (!regex_match(file_entry->d_name, std::regex("[\.]+"))) {
                            if (provide_full_path) {
                                filename.append(data_path).append("/");
                            }
                            filename.append(objective_dir)
                            .append("/")
                            .append(target_name)
                            .append("/")
                            .append(file_entry->d_name);
                            res.push_back(filename);
                        }
                    }
                }
            }
        }else {
            DIR* root_dir_open = opendir(data_path.c_str());
            struct dirent *root_entry;
            while((root_entry = readdir(root_dir_open)) != nullptr) {
                if (!regex_match(root_entry->d_name, std::regex("[\.]+"))) {
                    std::string filename;
                    if (provide_full_path) {
                        filename.append(data_path).append("/");
                    }
                    filename.append(root_entry->d_name);
                    res.push_back(filename);
                }
            }
            break;
        }
    }
    return res;
}


void run_transparent(const std::string& data_path, int epochs, bool partial_read){
    auto values = get_filenames(data_path, true);
    int i = 0;
    int max_size = 10000000;
    for(int k = 0; k < epochs; k++) {
        for (const auto &v : values) {
            int fd = open(v.c_str(), O_RDONLY);
            std::cout << "reading: " << v << "\n";
            void* buff;
            int ic = 20000;
            if (partial_read) {
                size_t n = 1;
                for (int j = 0; j <= max_size && n > 0; j += ic) {
                    if(j == ic)
                        sleep(1);
                    buff = mmap(NULL, ic, PROT_READ,MAP_SHARED, fd, j);
                }
            } else {
                buff = mmap(NULL, ic, PROT_READ,MAP_SHARED, fd, 0);
            }
            i++;
            int err = munmap(buff, ic);
            close(fd);
            usleep(100000);
        }
    }
    //random operations
    int fd = open("/etc/hosts", O_RDONLY|O_CLOEXEC);
    void* buff;
    buff = mmap(NULL, max_size, PROT_READ|PROT_WRITE,MAP_SHARED, fd, 0);
    close(fd);
}

int main (int argc, char** argv){
    run_transparent("/home/dantas/storage_backend_test/lustre/tf_data", 2, false);
}
