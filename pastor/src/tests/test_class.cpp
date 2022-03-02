//
// Created by dantas on 10/11/20.
//

#include <iostream>
#include <dirent.h>
#include <regex>

#include "test_class.h"
#include "../data_plane/interfaces/transparent/tmonarch.h"
#include "../data_plane/interfaces/intrusive/imonarch.h"
#include "../data_plane/logic/remote/pytorch_multiprocess/us_client.h"
#include "../data_plane/logic/remote/pytorch_multiprocess/us_server.h"

std::vector<std::string> TestClass::get_filenames(const std::string& data_path, bool provide_full_path){
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
                std::string filename;
                if (!regex_match(root_entry->d_name, std::regex("[\.]+"))) {
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

void TestClass::run_intrusive_instance(const std::string& data_path, int epochs, bool provide_full_path, bool partial_reads){
    auto* dpi = new IMonarch();
    dpi->standalone_init();
    dpi->start();
    auto values = get_filenames(data_path, provide_full_path);
    int i = 0;
    for(int k = 0; k < epochs; k++) {
        for (const auto &v : values) {
            std::string filename;
            if (provide_full_path) {
                filename = dpi->decode_filename(v).data();
            } else {
                filename = v;
            }
            size_t size = dpi->get_file_size(filename);
            std::cout << "reading: " << filename << " ,size: " << size << " ,i: " << i << "\n";
            char buff[size];
            if (partial_reads) {
                for (int j = 0; j <= size; j += 10000) {
                    dpi->read(filename, buff + j, j, 10000);
                }
            } else {
                dpi->read(filename, buff, 0, size);
            }
            i++;
            sleep(1);
        }
    }
    delete dpi;
}

#if defined(INCLUDE_GRPC)
void TestClass::run_instance(BootstrapClient* ec, const std::string &c_server_addr, const std::string &dp_server_addr){
    auto* dpi = new IMonarch(1, 1);
    dpi->server_attach_init(ec->get_group(), c_server_addr, dp_server_addr);
    dpi->start();
    std::vector<std::string> values = ec->get_filenames();
    int i = 0;
    for (const std::string& v : values){
        size_t size = dpi->get_file_size(v);
        std::cout << "reading: " << v << " ,size: " << size << " ,i: " << i << "\n";
        char buff[size];
        dpi->read(v, buff, 0, size);
        i++;
    }
}

void TestClass::run_instance_tf(BootstrapClient* ec, const std::string &c_server_addr, const std::string &dp_server_addr){
    auto* dpi = IMonarch::get_instance(c_server_addr);
    dpi = IMonarch::get_instance(c_server_addr);
    std::vector<std::string> values = ec->get_filenames_full_path();
    std::cout << "List size: " << values.size() << " !!!!" << std::endl;
    int i = 0;
    for (const auto& v : values){
        auto filename = dpi->decode_filename(v);
        long size = dpi->get_file_size(filename.data());
        std::cout << "reading: " << v << " ,size: " << size << " ,i: " << i << "\n";
        char buff[size];
        for(int j = 0; j <= size; j+= 10000) {
            dpi->read(filename.data(), buff + j, j, 10000);
        }
        i++;
        //if(i % 100 == 0)
            //sleep(3);
    }
}


void TestClass::run_instance(int rank, BootstrapClient* ec, const std::string &c_server_addr, const std::string &dp_server_addr){
    auto* dpi = new IMonarch(rank, 0);
    dpi->server_attach_init(ec->get_group(), c_server_addr, dp_server_addr);
    dpi->start();
    std::vector<int> values = ec->get_ids_from_rank(rank);
    std::cout << "List size: " << values.size() << " !!!!" << std::endl;
    int i = 0;
    for (auto v : values){
        long size = dpi->get_file_size_from_id(v);
        int target = dpi->get_target_class_from_id(v);
        std::cout << "reading: " << v << " ,size: " << size << " ,i: " << i << " target:" << target << "\n";
        char buff[size];
        dpi->read_from_id(v, buff);
        i++;
    }
}

class thread_server{
public:
    void operator()()
    {
        auto* server = new USServer();
        server->start();
    }
};

void TestClass::run_instance_pytorch(int rank, BootstrapClient* ec, const std::string &c_server_addr, const std::string &dp_server_addr){
    std::thread t_server((thread_server()));
    sleep(2);
    auto* cli = new USClient();
    cli->bind(rank, ec->get_group(), c_server_addr, dp_server_addr);
    std::vector<int> values = ec->get_ids_from_rank(rank);
    std::cout << "List size: " << values.size() << " !!!!" << std::endl;
    int i = 0;
    for (auto v : values){
        size_t size = cli->prepare_transfer(v);
        int target = cli->get_target();
        std::cout << "reading: " << v << " ,size: " << size << " ,i: " << i << " target:" << target << "\n";
        char buff[size];
        cli->read(buff);
        i++;
    }
    t_server.join();
}

BootstrapClient* TestClass::run_ephemeral_client(const std::string& c_server_addr, const std::string& type, int number_of_workers, int world_size, bool return_names){

    auto* ec = new BootstrapClient(c_server_addr);
    ec->simple_request_session(type, world_size, number_of_workers, return_names);

    std::cout << "EphemeralClient retrieved ids" << std::endl;
    auto idx = ec->get_ids();
    for(auto& index : idx)
        std::cout << index << ",";
    std::cout << std::endl;

    for(int i = 0; i < world_size; i++) {
        auto idx0 = ec->get_ids_from_rank(i);
        std::cout << "Printing rank " << i << " list" << std::endl;
        for (auto &index : idx0)
            std::cout << index << ",";
        std::cout << std::endl;
    }
    return ec;
}
#endif