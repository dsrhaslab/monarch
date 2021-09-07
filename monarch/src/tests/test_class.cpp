//
// Created by dantas on 10/11/20.
//

#include "test_class.h"
#include "../data_plane/data_plane_instance.h"
#include "../data_plane/remote/pytorch_multiprocess/us_client.h"
#include "../data_plane/remote/pytorch_multiprocess/us_server.h"
#include <iostream>

void TestClass::run_instance(BootstrapClient* ec, const std::string &c_server_addr, const std::string &dp_server_addr){
    auto* dpi = new DataPlaneInstance(1,1);
    dpi->bind(ec->get_group(), c_server_addr, dp_server_addr);
    dpi->start();
    std::vector<std::string> values = ec->get_filenames();
    int i = 0;
    for (const std::string& v : values){
        int size = dpi->get_file_size(v);
        std::cout << "reading: " << v << " ,size: " << size << " ,i: " << i << "\n";
        char buff[size];
        dpi->read(v, buff, 0, size);
        i++;
    }
}

void TestClass::run_instance_tf(BootstrapClient* ec, const std::string &c_server_addr, const std::string &dp_server_addr){
    auto* dpi = DataPlaneInstance::get_instance(c_server_addr);
    dpi = DataPlaneInstance::get_instance(c_server_addr);
    std::vector<std::string> values = ec->get_filenames_full_path();
    std::cout << "List size: " << values.size() << " !!!!" << std::endl;
    int i = 0;
    for (const auto& v : values){
        auto filename = dpi->decode_filename(v);
        long size = dpi->get_file_size(filename);
        std::cout << "reading: " << v << " ,size: " << size << " ,i: " << i << "\n";
        char buff[size];
        for(int j = 0; j <= size; j+= 10000) {
            dpi->read(filename, buff + j, j, 10000);
        }
        i++;
        //if(i % 100 == 0)
            //sleep(3);
    }
}


void TestClass::run_instance(int rank, BootstrapClient* ec, const std::string &c_server_addr, const std::string &dp_server_addr){
    auto* dpi = new DataPlaneInstance(rank, 0);
    dpi->bind(ec->get_group(), c_server_addr, dp_server_addr);
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

/*
DataPlane* TestClass::build_data_plane(int option){
    switch (option) {
        case 1:
            return build_data_plane_opt1();
        case 2:
            return build_data_plane_opt2();
        case 3:
            return build_data_plane_opt3();
        default:
            return build_data_plane_opt1();
    }
}

DataPlane* TestClass::build_data_plane_opt1(){

    return HierarchicalDataPlane::create(2);
        .hierarchy()
            .with_storage(DataStorageDriver::create(DISK)
                .with_block_size(8192)
                .with_storage_prefix("/home/dantas/thesis/ssd")
            .with_storage(DataStorageDriver::create(DISK)
                .with_block_size(8192)
                .with_storage_prefix("/home/dantas/thesis/hdd")
                .with_max_storage_size(1000))
        .policies()
            .with_cache_insertion_policy(FIRST_LEVEL_ONLY)
            .with_cache_eviction_policy(FORWARD_DEMOTION);
}

DataPlane* TestClass::build_data_plane_opt2(){
    //std::string train_files_dir = "/home/dantas/thesis/hdd";

    std::string out_filename = "../aux_files/shuffle_filenames.txt";
    //FilenamesHandler::create_filenames_files(train_files_dir , out_filename, 1, ".*.txt");

    RootDataPlane* root_data_plane = HierarchicalDataPlane::create(2)
        .hierarchy()
            .with_storage(DataStorageDriver::create(DISK)
                .with_block_size(8192)
                .with_storage_prefix("/home/dantas/thesis/ssd")
                .with_max_storage_size(100))
            .with_storage(DataStorageDriver::create(DISK)
                .with_block_size(8192)
                .with_storage_prefix("/home/dantas/thesis/hdd")
                .with_max_storage_size(1000))
        .policies()
            .with_cache_insertion_policy(FIRST_LEVEL_ONLY)
            .with_cache_eviction_policy(FORWARD_DEMOTION);

    return PrefetchDataPlane::create(root_data_plane)
        .policies()
            .with_data_transfer_policy(LAST_TO_FIRST_LEVEL);
}

DataPlane* TestClass::build_data_plane_opt3(){
    //std::string train_files_dir = "/home/dantas/thesis/hdd";

    std::string out_filename = "../aux_files/shuffle_filenames.txt";
    //FilenamesHandler::create_filenames_files(train_files_dir , out_filename, 1, ".*.txt");

    RootDataPlane* root_data_plane = HierarchicalDataPlane::create(2)
        .hierarchy()
            .with_storage(DataStorageDriver::create(BLOCKING_MEMORY_BUFFER)
                .with_max_storage_size(100)
                .with_storage_access_policy(THREAD_SYNC_ON_WRITE))
            .with_storage(DataStorageDriver::create(DISK)
                .with_block_size(8192)
                .with_storage_prefix("/home/dantas/thesis/hdd")
                .with_max_storage_size(1000))
        .policies()
            .with_cache_insertion_policy(FIRST_LEVEL_ONLY)
            .with_cache_eviction_policy(FORWARD_DEMOTION);

    return PrefetchDataPlane::create(root_data_plane)
            .policies()
                .with_data_transfer_policy(LAST_TO_FIRST_LEVEL);
}
 */