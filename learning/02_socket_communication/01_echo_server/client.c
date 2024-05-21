#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <err.h>

#define SERVER_ADDR     "127.0.0.1"
#define SERVER_PORT     50001
#define MAX_BUFFER_SIZE 1024

int main(void)
{
    int sock;
    struct sockaddr_in sock_addr;
    char recv_buffer[MAX_BUFFER_SIZE];
    char send_buffer[MAX_BUFFER_SIZE];
    int send_size, recv_size;
    int length;

    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
    sock_addr.sin_port = htons(SERVER_PORT);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        err(EXIT_FAILURE, "socket() failed");
    }

    if (connect(sock, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) < 0) {
        err(EXIT_FAILURE, "connect() failed");
    }

    while (1) {
        printf("Please enter the characters:");

        memset(send_buffer, 0, sizeof(send_buffer));
        if (fgets(send_buffer, sizeof(send_buffer), stdin) == NULL) {
            perror("fgets() failed");
            break;
        }

        length = strlen(send_buffer);
        if (length == MAX_BUFFER_SIZE - 1) {
            char c;
            while ((c = getchar()) != '\n' && c != EOF);
            send_buffer[length - 2] = '\n';
            send_buffer[length - 1] = '\0';
        }

        send_size = send(sock, send_buffer, length, 0);
        if (send_size < 0) {
            perror("send() failed");
            break;
        }

        printf("send:%s", send_buffer);

        memset(recv_buffer, 0, sizeof(recv_buffer));
        recv_size = recv(sock, recv_buffer, sizeof(recv_buffer), 0);
        if (recv_size < 0) {
            perror("recv() failed");
            break;
        }

        printf("receive:%s", recv_buffer);
    }

    close(sock);

    exit(EXIT_SUCCESS);
}