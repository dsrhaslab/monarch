//
// Created by dantas on 10/11/20.
//

#ifndef THESIS_TEST_CLASS_H
#define THESIS_TEST_CLASS_H

#include "../data_plane/logic/data_plane.h"
#include <vector>
#include "../data_plane/logic/remote/bootstrap_client.h"

class TestClass {
    DataPlane* data_plane;

    std::vector<std::string> get_filenames(const std::string& data_path, bool provide_full_path);
public:
    void run_intrusive_instance(const std::string& data_path, int epochs, bool provide_full_path, bool partial_read);

#if defined(INCLUDE_GRPC)
    BootstrapClient* run_ephemeral_client(const std::string& c_server_addr, const std::string& type, int number_of_workers, int world_size, bool return_names);
    void run_instance(BootstrapClient* ec, const std::string &c_server_addr, const std::string &dp_server_addr);
    void run_instance_tf(BootstrapClient* ec, const std::string &c_server_addr, const std::string &dp_server_addr);
    void run_instance_pytorch(int rank, BootstrapClient* ec, const std::string &c_server_addr, const std::string &dp_server_addr);
    void run_instance(int rank, BootstrapClient* ec, const std::string &c_server_addr, const std::string &dp_server_addr);
#endif
};

#endif //THESIS_TEST_CLASS_H
