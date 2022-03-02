//
// Created by dantas on 13/05/21.
//

#include <netinet/in.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>
#include <iostream>

#include "us_server.h"
#include "us_client.h"

#include "../../../interfaces/intrusive/imonarch.h"

void USServer::create_connection(){
    // Creating socket file descriptor
    const char* socket_name = "server-socket";
    unlink (socket_name);

    if ((server_fd = socket (AF_UNIX, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Socket creation error: " << strerror(errno) << std::endl;
        exit(1);
    } else {
        std::cout << "Server socket created." << std::endl;
    }

    unix_socket.sun_family = AF_UNIX;
    strncpy (unix_socket.sun_path, socket_name, sizeof(unix_socket.sun_path) - 1);

    if (bind (server_fd, (struct sockaddr*) &unix_socket, sizeof(unix_socket)) < 0) {
        std::cerr << "Bind error: " << strerror(errno) << std::endl;
        exit(1);
    }

    if (listen (server_fd, 50) < 0) {
        std::cerr << "Listen error: " << strerror(errno) << std::endl;
        exit(1);
    } else {
        std::cout << "Server listening..." << std::endl;
    }

    addr_len = sizeof (unix_socket);
}

int USServer::accept(){
    int new_socket_t = -1;
    // Accept new PRISMA client connection
    new_socket_t = ::accept(server_fd, (struct sockaddr*) &unix_socket, (socklen_t *) &addr_len);

    // Verify socket value
    if (new_socket_t == -1) {
        std::cerr << "Failed to connect with client." << std::endl;
    } else {
        std::cout << "New client connection established." << std::endl;
    }

    return new_socket_t;
}


void USServer::serialize_int(unsigned char *buffer, int value){
    buffer[3] = value >> 24;
    buffer[2] = value >> 16;
    buffer[1] = value >> 8;
    buffer[0] = value;
}

void USServer::serialize_size_t(unsigned char *buffer, size_t value){
    buffer[7] = value >> 56;
    buffer[6] = value >> 48;
    buffer[5] = value >> 40;
    buffer[4] = value >> 32;
    buffer[3] = value >> 24;
    buffer[2] = value >> 16;
    buffer[1] = value >> 8;
    buffer[0] = value;
}


int check_for_endianness(){
    unsigned int x = 0x76543210;
    char *c = (char*) &x;

    printf ("*c is: 0x%x\n", *c);
    if (*c == 0x10)
    {
        printf ("Underlying architecture is little endian. \n");
    }
    else
    {
        printf ("Underlying architecture is big endian. \n");
    }

    return 0;
}

void USServer::handle_request(int sock) {
    IMonarch* data_plane;
    check_for_endianness();
    int res;
    int return_value_t;
#if defined(INCLUDE_GRPC)
    //Bind info
    USClient::bind_info bi{};
    res = ::read(sock, &bi, sizeof(bi));
    if(res != sizeof(struct USClient::bind_info)) {
        std::cerr << "Failed to receive client bind info: " << strerror(errno) << std::endl;
        close(sock);
        return;
    }

    data_plane = IMonarch::get_instance(bi.rank, bi.group, bi.c_server_addr, bi.dp_server_addr);

    // Send ack to unblock
    bind_reply br{};
    br.ok = 0;
    return_value_t = ::write(sock, &br, sizeof(br));
    if (return_value_t != sizeof(struct bind_reply)) {
        std::cerr << "Failed to send bind reply confirmation: " << strerror(errno) << std::endl;
        exit(1);
    }
#else
    data_plane = IMonarch::get_instance();
#endif

    // Read request
    USClient::transfer_request req{};
    res = ::read(sock, &req, sizeof(req));
    if (res != sizeof(struct USClient::transfer_request)) {
        std::cerr << "Failed to receive client transfer request: " << strerror(errno) << std::endl;
        close(sock);
        return;
    }

    while(res > 0) {
        // Read data from buffer
        size_t n_bytes = data_plane->get_file_size_from_id(req.id);
        int target = data_plane->get_target_class_from_id(req.id);
        size_t header_size = sizeof(int) + sizeof(size_t);
        unsigned char* result_buffer = new unsigned char[header_size + n_bytes];
        bzero(result_buffer, n_bytes + header_size);
        //define file size
        serialize_size_t(result_buffer, n_bytes);
        //define target class
        serialize_int(result_buffer + sizeof(size_t), target);
        //fill file content
        int r = data_plane->read_from_id(req.id, reinterpret_cast<char*>(result_buffer + header_size));
        if (r == 0) {
            std::cerr << "Error while reading file with id " << req.id << std::endl;
        }

        return_value_t = ::write(sock, result_buffer, r + header_size);
        if (return_value_t != r + header_size) {
            std::cerr << "Failed to send server response: " << strerror(errno) << std::endl;
            exit(1);
        }

        res = ::read(sock, &req, sizeof(struct USClient::transfer_request));
        if (res != sizeof(struct USClient::transfer_request)) {
            std::cerr << "Failed to receive client request: " << strerror(errno) << std::endl;
            close(sock);
            return;
        }

        // Free memory
        delete[] result_buffer;
    }
}

[[noreturn]] void USServer::start_loop(){
    // Prepare UNIX connection
    create_connection();

    while(1) {
        std::cout << "Waiting for client connection ..." << std::endl;
        // Accept PRISMA client connection
        int socket_t = accept();

        if (socket_t != -1) {
            // Start handler thread for this connection
            std::thread handler_thread(&USServer::handle_request, this, socket_t);
            handler_thread.detach();

            std::cout << "Connecting and going for the read_from_id one ..." << std::endl;
        }
    }
}

void USServer::start(){
    start_loop();
}

