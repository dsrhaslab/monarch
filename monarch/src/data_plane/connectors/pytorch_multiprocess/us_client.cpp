//
// Created by dantas on 13/05/21.
//

#include <netinet/in.h>
#include <sys/un.h>
#include <unistd.h>
#include <atomic>
#include <iostream>
#include <cstring>

#include "us_client.h"
#include "us_server.h"

USClient::USClient() {
    // Create UNIX connection with the Pastor server
    aux_buffer = new unsigned char[MAX_BUFFER_SIZE];
    header_length = sizeof(int) + sizeof(size_t);
    create_connection();
}

void USClient::create_connection(){
    unix_socket.sun_family = AF_UNIX;

    // Create socket name
    char socket_name[] = "server-socket";
    strncpy(unix_socket.sun_path, socket_name, sizeof(unix_socket.sun_path) - 1);

    // Create socket
    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Client socket creation error: " << strerror(errno) << std::endl;
        exit(1);
    } else {
        std::cout << "Client socket created." << std::endl;
    }

    // Establish connection with PRISMA server
    int max_retries = 3;
    int retries = 0;
    while(::connect (sock, (const struct sockaddr *) &unix_socket, sizeof(struct sockaddr_un)) < 0 && retries < 3){
        std::cerr << "Connection failed: " << strerror(errno) <<  " Retrying " << retries << " ..." << std::endl;
        retries++;
        sleep(5);
    }
    if(retries == max_retries){
        std::cerr << "Connection failed: " << strerror(errno) <<  " Max retries reached." << std::endl;
        exit(1);
    }
    std::cout << "Client connection created for " << socket_name << "." << std::endl;
}

#if defined(INCLUDE_GRPC)
void USClient::bind(int rank, const std::string &group, const std::string &c_server_addr, const std::string &dp_server_addr){
    bind_info bi{};
    bi.rank = rank;
    strcpy(bi.group, group.c_str());
    strcpy(bi.c_server_addr, c_server_addr.c_str());
    strcpy(bi.dp_server_addr, dp_server_addr.c_str());

    int return_value_t = ::write(sock, &bi, sizeof(struct bind_info));

    if (return_value_t != sizeof(struct bind_info)) {
        std::cerr << "Failed to send bind info: " << strerror(errno) << std::endl;
        close(sock);
        exit(1);
    }

    USServer::bind_reply br{};
    return_value_t = ::read(sock, &br, sizeof(br));
    if (return_value_t != sizeof(struct  USServer::bind_reply)) {
        std::cerr << "Failed to receive server bind reply: " << strerror(errno) << std::endl;
        close(sock);
        exit(1);
    }
}
#endif

int USClient::deserialize_int(unsigned char *buffer){
    int value = 0;

    value |= buffer[3] << 24;
    value |= buffer[2] << 16;
    value |= buffer[1] << 8;
    value |= buffer[0];
    return value;
}

size_t USClient::deserialize_size_t(unsigned char *buffer){
    size_t value = 0;
    value |= (size_t)buffer[0];
    value |= (size_t)buffer[1] << 8;
    value |= (size_t)buffer[2] << 16;
    value |= (size_t)buffer[3] << 24;
    value |= (size_t)buffer[4] << 32;
    value |= (size_t)buffer[5] << 40;
    value |= (size_t)buffer[6] << 48;
    value |= (size_t)buffer[7] << 56;
    return value;
}

//returns size
size_t USClient::prepare_transfer(int id){
    transfer_request req{};
    id_ = id;
    req.id = id;

    int return_value_t = ::write(sock, &req, sizeof(struct transfer_request));
    if (return_value_t != sizeof(struct transfer_request)) {
        std::cerr << "Failed to send client transfer request: " << strerror(errno) << std::endl;
        close(sock);
        exit(1);
    }

    return_value_t = ::read(sock, aux_buffer, MAX_BUFFER_SIZE);
    if (return_value_t < header_length) {
        std::cerr << "Failed to read transfer header " << strerror(errno) << std::endl;
        close(sock);
        exit(1);
    }

    size = deserialize_size_t(aux_buffer);
    target = deserialize_int(aux_buffer + sizeof(size_t));
    buffered_size = return_value_t - header_length;
    return size;
}

int USClient::get_target(){
    return target;
}

ssize_t USClient::read(char *result) {
    memcpy(result, aux_buffer + header_length, buffered_size);
    int bytes_read = buffered_size;
    if(buffered_size < size){
        int return_value_t;
        while (bytes_read < size) {
            return_value_t = ::read(sock, result + bytes_read, size - bytes_read);
            if (return_value_t <= 0) {
                std::cerr << "Error while reading file with id " << id_ << std::endl;
                exit(1);
            }

            // Save content in result array
            bytes_read += return_value_t;
        }
    }
    return bytes_read;
}
