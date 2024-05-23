#include "common.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int create_server_socket(int port, int *server_sock)
{
    struct sockaddr_in server_addr;
    int opt = 1;

    *server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (*server_sock < 0) {
        perror("server socket() failed");
        return RET_NG;
    }

    // SO_REUSEADDR オプションを設定
    if (setsockopt(*server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt() failed");
        close(*server_sock);
        return RET_NG;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(*server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind() failed");
        close(*server_sock);
        return RET_NG;
    }

    if (listen(*server_sock, MAX_QUEUE_SIZE) < 0) {
        perror("listen() failed");
        close(*server_sock);
        return RET_NG;
    }

    return RET_OK;
}

int create_client_socket(const char *server_addr, int port, int *client_sock)
{
    struct sockaddr_in sock_addr;

    *client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (*client_sock < 0) {
        perror("client socket() failed");
        return RET_NG;
    }

    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = inet_addr(server_addr);
    sock_addr.sin_port = htons(port);

    if (connect(*client_sock, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) < 0) {
        perror("connect() failed");
        close(*client_sock);
        return RET_NG;
    }

    return RET_OK;
}

int send_data(int sock, const void *data, int length)
{
    int total_sent = 0;
    while (total_sent < length) {
        int send_size = send(sock, (const char *)data + total_sent, length - total_sent, 0);
        if (send_size < 0) {
            perror("send() failed");
            return RET_NG;
        }
        total_sent += send_size;
    }

    return total_sent;
}

int recv_data(int sock, void *buffer, int buffer_size)
{
    int total_received = 0;
    while (total_received < buffer_size) {
        int recv_size = recv(sock, (char *)buffer + total_received, buffer_size - total_received, 0);
        if (recv_size < 0) {
            perror("recv() failed");
            return RET_NG;
        } else if (recv_size == 0) {
            break;
        }
        total_received += recv_size;
    }

    return total_received;
}
