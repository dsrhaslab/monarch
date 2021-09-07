#include "tests/test_class.h"

#include <vector>
#include <iostream>

int main() {
    auto *tc = new TestClass();
    std::string server_addr = "0.0.0.0:50051";
    std::string dp_server_addr = "0.0.0.0:20001";
    int num_workers = 1;
    int world_size = 1;
    auto *ec = tc->run_ephemeral_client(server_addr, "test", num_workers, world_size, true);
    //tc->run_instance(0, ec, server_addr, dp_server_addr);
    tc->run_instance_tf(ec, server_addr, dp_server_addr);
    //tc->run_instance_pytorch(0, ec, server_addr, dp_server_addr);
}