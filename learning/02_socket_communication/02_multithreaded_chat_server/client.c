#include "common.h"

int get_line(char *buffer, int buffer_size)
{
    int length;

    memset(buffer, 0, buffer_size);
    if (fgets(buffer, buffer_size, stdin) == NULL) {
        perror("fgets() failed");
        return 0;
    }

    length = strlen(buffer);
    if (length == buffer_size - 1) {
        char c;
        while ((c = getchar()) != '\n' && c != EOF);
        buffer[length - 2] = '\n';
        buffer[length - 1] = '\0';
    }

    return length;
}

void receive_history(int socket)
{
    ChatMessage message;
    while (1) {
        memset(&message, 0, sizeof(ChatMessage));
        if (recv_data(socket, &message, sizeof(ChatMessage)) <= 0) {
            break;
        }
        if (strncmp(message.command, "history", 7) != 0) {
            break;
        }
        printf("%s", message.message);
    }
}

int main(int argc, char *argv[])
{
    int socket;
    fd_set readfds;
    ChatMessage message;
    int send_size, recv_size;
    int length;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP address> <Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    if (create_client_socket(ip, port, &socket) != RET_OK) {
        exit(EXIT_FAILURE);
    }

    // ユーザー認証（ユーザー名の送信）
    printf("Please enter your username: ");
    get_line(message.from_user, sizeof(message.from_user));
    strncpy(message.command, "auth", sizeof(message.command) - 1);
    send_size = send_data(socket, &message, sizeof(ChatMessage));
    if (send_size < 0) {
        perror("send() failed");
        close(socket);
        exit(EXIT_FAILURE);
    }

    // サーバーからの認証応答を受信
    recv_size = recv_data(socket, &message, sizeof(ChatMessage));
    if (recv_size <= 0 || strncmp(message.command, "auth_ok", 7) != 0) {
        perror("Authentication failed");
        close(socket);
        exit(EXIT_FAILURE);
    }

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(socket, &readfds);
        FD_SET(STDIN_FILENO, &readfds);

        int activity = select(socket + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            perror("select() failed");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            printf("Please enter the characters:");

            length = get_line(message.message, sizeof(message.message));
            if (length == 0) {
                break;
            }

            if (strncmp(message.message, "/history", 8) == 0) {
                strncpy(message.command, "history", sizeof(message.command) - 1);
                send_size = send_data(socket, &message, sizeof(ChatMessage));
                if (send_size < 0) {
                    perror("send() failed");
                    break;
                }

                receive_history(socket);
            } else if (strncmp(message.message, "/list", 5) == 0) {
                strncpy(message.command, "list", sizeof(message.command) - 1);
                send_size = send_data(socket, &message, sizeof(ChatMessage));
                if (send_size < 0) {
                    perror("send() failed");
                    break;
                }

                memset(&message, 0, sizeof(ChatMessage));
                recv_size = recv_data(socket, &message, sizeof(ChatMessage));
                if (recv_size < 0) {
                    perror("recv() failed");
                    break;
                } else if (recv_size == 0) {
                    printf("connection closed\n");
                    break;
                } else {
                    printf("%s", message.message);
                }
            } else if (strncmp(message.message, "/quit", 5) == 0) {
                strncpy(message.command, "quit", sizeof(message.command) - 1);
                send_size = send_data(socket, &message, sizeof(ChatMessage));
                if (send_size < 0) {
                    perror("send() failed");
                    break;
                }
                break;
            } else {
                strncpy(message.command, "message", sizeof(message.command) - 1);
                send_size = send_data(socket, &message, sizeof(ChatMessage));
                if (send_size < 0) {
                    perror("send() failed");
                    break;
                } else if (send_size == 0) {
                    printf("connection closed\n");
                    break;
                } else {
                    printf("send: %s", message.message);
                }
            }
        }

        if (FD_ISSET(socket, &readfds)) {
            memset(&message, 0, sizeof(ChatMessage));
            recv_size = recv_data(socket, &message, sizeof(ChatMessage));
            if (recv_size < 0) {
                perror("recv() failed");
                break;
            } else if (recv_size == 0) {
                printf("connection closed\n");
                break;
            } else {
                printf("receive: %s", message.message);
            }
        }
    }

    close(socket);

    exit(EXIT_SUCCESS);
}
