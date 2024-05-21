
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <err.h>

#define SERVER_PORT     50001
#define MAX_BUFFER_SIZE 1024    
#define MAX_QUEUE_SIZE  5

int main(void)
{
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_length;
    char buffer[MAX_BUFFER_SIZE];
    int recv_size, send_size;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        err(EXIT_FAILURE, "socket() failed");
    }

    memset((char *)&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        err(EXIT_FAILURE, "bind() failed");
    }

    if(listen(server_sock, MAX_QUEUE_SIZE) < 0) {
        err(EXIT_FAILURE, "listen() failed");
    }
    
    while (1) {
        client_length = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_length);
        if (client_sock < 0) {
            perror("accept() failed");
            break;
        }

        printf("connected from %s\n", inet_ntoa(client_addr.sin_addr));
        
        while (1) {
            memset(buffer, 0, sizeof(buffer));
            recv_size = recv(client_sock, buffer, sizeof(buffer), 0);
            if (recv_size < 0) {
                perror("recv() failed");
                break;
            } else if (recv_size == 0) {
                printf("connection closed\n");
                break;
            } else {
                printf("receive:%s", buffer);
            }

            send_size = send(client_sock, buffer, recv_size, 0);
            if (send_size < 0) {
                perror("send() failed");
                break;
            } else if (send_size == 0) {
                printf("connection closed\n");
                break;
            } else {
                printf("send:%s", buffer);
            }
        }

        close(client_sock);
    }

    close(server_sock);

    exit(EXIT_SUCCESS);
}