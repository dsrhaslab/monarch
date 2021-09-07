//
// Created by dantas on 13/05/21.
//

#ifndef THESIS_US_SERVER_H
#define THESIS_US_SERVER_H

#include <sys/un.h>

class USServer {
private:
    struct sockaddr_un unix_socket;
    int server_fd;
    int addr_len;

    void create_connection();
    int accept();
    void handle_request(int sock);

    [[noreturn]] void start_loop();

    void serialize_int(unsigned char *buffer, int value);
    void serialize_size_t(unsigned char *buffer, size_t value);

public:
    USServer(){};
    ~USServer(){}


    struct bind_reply {
        int ok;
    };

    void start();
};

#endif //THESIS_US_SERVER_H
