//
// Created by dantas on 13/05/21.
//

#ifndef THESIS_US_CLIENT_H
#define THESIS_US_CLIENT_H

#include <sys/un.h>
#include <string>

#define MAX_FILENAME_SIZE 1000
#define MAX_GENERAL_STRING_SIZE 100
#define MAX_BUFFER_SIZE 262144

class USClient {
private:
    int sock;
    struct sockaddr_un unix_socket;
    size_t header_length;
    //from current request
    unsigned char* aux_buffer;
    size_t buffered_size;
    int id_;
    int target;
    size_t size;

    void create_connection();

public:

#if defined(INCLUDE_GRPC)
    struct bind_info {
        int rank;
        char group[MAX_GENERAL_STRING_SIZE];
        char c_server_addr[MAX_GENERAL_STRING_SIZE];
        char dp_server_addr[MAX_GENERAL_STRING_SIZE];
    };

    void bind(int rank, const std::string &group, const std::string &c_server_addr, const std::string &dp_server_addr);
#endif
    struct transfer_request {
        int id;
    };

    static int deserialize_int(unsigned char *buffer);
    static size_t deserialize_size_t(unsigned char *buffer);
    USClient();
    ~USClient(){}

    //returns size
    size_t prepare_transfer(int id);
    int get_target();
    ssize_t read(char* result);
};

#endif //THESIS_US_CLIENT_H
