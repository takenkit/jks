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

void clear_line()
{
    printf("\r\033[K");
    fflush(stdout);
}

void receive_history(int socket)
{
    ChatMessage message;
    while (1) {
        memset(&message, 0, sizeof(ChatMessage));
        if (recv_data(socket, &message, sizeof(ChatMessage)) <= 0) {
            break;
        }
        if (strncmp(message.command, "history", 8) == 0) {
            printf("%s", message.text);
        } else if (strncmp(message.command, "history_end", 12) == 0) {
            break;
        } else {
            break;
        }
    }
}

int send_chat_message(int sock, const char *command, const char *from_user, const char *to_user, const char *message_text)
{
    ChatMessage message;
    struct timespec ts;

    memset(&message, 0, sizeof(ChatMessage));

    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm *tm_info = localtime(&ts.tv_sec);
    message.year = tm_info->tm_year + 1900;
    message.month = tm_info->tm_mon + 1;
    message.day = tm_info->tm_mday;
    message.hour = tm_info->tm_hour;
    message.minute = tm_info->tm_min;
    message.second = tm_info->tm_sec;
    message.millisecond = ts.tv_nsec / 10000000; // 10ミリ秒単位
    strncpy(message.from_user, from_user, sizeof(message.from_user) - 1);
    if (to_user != NULL) {
        strncpy(message.to_user, to_user, sizeof(message.to_user) - 1);
    }
    strncpy(message.command, command, sizeof(message.command) - 1);
    if (message_text != NULL) {
        strncpy(message.text, message_text, sizeof(message.text) - 1);
    }

    return send_data(sock, &message, sizeof(ChatMessage));
}

int main(int argc, char *argv[])
{
    int socket;
    fd_set readfds;
    ChatMessage message;
    int send_size, recv_size;
    int length;
    char username[MAX_USERNAME_LENGTH];
    char buffer[MAX_BUFFER_SIZE];

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
    printf("Welcome to the chat server!\n");
    printf("Please enter your username: ");
    get_line(username, sizeof(username));
    username[strlen(username) - 1] = '\0';
    send_size = send_chat_message(socket, "auth", username, NULL, NULL);
    if (send_size < 0) {
        perror("send() failed");
        close(socket);
        exit(EXIT_FAILURE);
    }

    // サーバーからの認証応答を受信
    recv_size = recv_data(socket, &message, sizeof(ChatMessage));
    if (recv_size <= 0 || strncmp(message.command, "auth_ok", 7) != 0) {
        perror("Authentication failed");
        perror(message.text);
        close(socket);
        exit(EXIT_FAILURE);
    }

    printf("%s", message.text);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(socket, &readfds);
        FD_SET(STDIN_FILENO, &readfds);

        clear_line();
        printf("[%s] > ", username);
        fflush(stdout);

        int activity = select(socket + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            clear_line();
            perror("select() failed");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            length = get_line(buffer, sizeof(buffer));
            if (length == 0) {
                break;
            }

            if (strncmp(buffer, "/history", 8) == 0) {
                send_size = send_chat_message(socket, "history", username, NULL, NULL);
                if (send_size < 0) {
                    clear_line();
                    perror("send() failed");
                    break;
                }

                receive_history(socket);
            } else if (strncmp(buffer, "/list", 5) == 0) {
                send_size = send_chat_message(socket, "list", username, NULL, NULL); 
                if (send_size < 0) {
                    clear_line();
                    perror("send() failed");
                    break;
                }

                memset(&message, 0, sizeof(ChatMessage));
                recv_size = recv_data(socket, &message, sizeof(ChatMessage));
                if (recv_size < 0) {
                    clear_line();
                    perror("recv() failed");
                    break;
                } else if (recv_size == 0) {
                    clear_line();
                    printf("Connection closed\n");
                    break;
                } else {
                    printf("%s", message.text);
                }
            } else if (strncmp(buffer, "/quit", 5) == 0) {
                send_size = send_chat_message(socket, "quit", username, NULL, NULL);
                if (send_size < 0) {
                    clear_line();
                    perror("send() failed");
                    break;
                }
                printf("Goodbye!\n");
                break;
            } else if (strncmp(buffer, "/msg", 4) == 0) {
                char text[MAX_BUFFER_SIZE];
                char to_user[MAX_USERNAME_LENGTH];
                sscanf(buffer, "/msg %s %[^n]", to_user, text);
                send_size = send_chat_message(socket, "private", username, to_user, text);
                if (send_size < 0) {
                    clear_line();
                    perror("send() failed");
                    break;
                } else {
                    printf("Send: %s", text);
                }
            } else {
                send_size = send_chat_message(socket, "message", username, NULL, buffer); 
                if (send_size < 0) {
                    clear_line();
                    perror("send() failed");
                    break;
                } else if (send_size == 0) {
                    clear_line();
                    printf("Connection closed\n");
                    break;
                } else {
                    printf("Send: %s", buffer);
                }
            }
        }

        if (FD_ISSET(socket, &readfds)) {
            memset(&message, 0, sizeof(ChatMessage));
            recv_size = recv_data(socket, &message, sizeof(ChatMessage));
            if (recv_size < 0) {
                clear_line();
                perror("recv() failed");
                break;
            } else if (recv_size == 0) {
                clear_line();
                printf("Connection closed\n");
                break;
            } else {
                clear_line();
                if (strncmp(message.command, "info", 5) == 0 || strncmp(message.command, "error", 6) == 0) {
                    printf("%s", message.text);
                } else {
                    printf("Receive: <%s>: %s", message.from_user, message.text);
                }
            }
        }
    }

    close(socket);

    exit(EXIT_SUCCESS);
}
