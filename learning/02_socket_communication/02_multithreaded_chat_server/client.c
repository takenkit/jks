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
        } else if (strncmp(message.command, "error", 6) == 0) {
            fprintf(stderr, "%s", message.text);
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

int receive_message(int socket)
{
    ChatMessage message;
    
    int recv_size = recv_data(socket, &message, sizeof(ChatMessage));
    if (recv_size < 0) {
        clear_line();
        perror("recv_data() failed");
        return RET_NG;
    } else if (recv_size == 0) {
        clear_line();
        printf("Connection closed\n");
        return RET_NG;
    } else {
        clear_line();
        if (strncmp(message.command, "message", 7) == 0) {
            printf("[%02d:%02d:%02d] <%s>: %s", message.hour, message.minute, message.second, message.from_user, message.text);
        } else if (strncmp(message.command, "private", 7) == 0) {
            printf("[%02d:%02d:%02d] [Private] <%s> to <%s>: %s", message.hour, message.minute, message.second, message.from_user, message.to_user, message.text);
        } else if (strncmp(message.command, "info", 4) == 0 || strncmp(message.command, "error", 5) == 0) {
            printf("[%02d:%02d:%02d] %s", message.hour, message.minute, message.second, message.text);
        }
    }

    return RET_OK;
}

void handle_history_command(int socket, const char *username)
{
    int send_size = send_chat_message(socket, "history", username, NULL, NULL);
    if (send_size < 0) {
        clear_line();
        perror("send() failed");
        return;
    }
    receive_history(socket);
}

void handle_list_command(int socket, const char *username)
{
    ChatMessage message;
    memset(&message, 0, sizeof(ChatMessage));
    int send_size = send_chat_message(socket, "list", username, NULL, NULL);
    if (send_size < 0) {
        clear_line();
        perror("send() failed");
        return;
    }
    int recv_size = recv_data(socket, &message, sizeof(ChatMessage));
    if (recv_size < 0) {
        clear_line();
        perror("recv() failed");
    } else if (recv_size == 0) {
        clear_line();
        printf("Connection closed\n");
    } else {
        printf("%s", message.text);
    }
}

void handle_quit_command(int socket, const char *username)
{
    int send_size = send_chat_message(socket, "quit", username, NULL, NULL);
    if (send_size < 0) {
        clear_line();
        perror("send() failed");
        return;
    }
    printf("Goodbye!\n");
}

void handle_msg_command(int socket, const char *username, const char *buffer)
{
    char text[MAX_BUFFER_SIZE];
    char to_user[MAX_USERNAME_LENGTH];
    sscanf(buffer, "/msg %s %[^n]", to_user, text);
    int send_size = send_chat_message(socket, "private", username, to_user, text);
    if (send_size < 0) {
        clear_line();
        perror("send() failed");
    } else {
        printf("Send: <%s> to <%s>: %s", username, to_user, text);
    }
}

void handle_regular_message(int socket, const char *username, const char *buffer)
{
    int send_size = send_chat_message(socket, "message", username, NULL, buffer);
    if (send_size < 0) {
        clear_line();
        perror("send() failed");
    } else if (send_size == 0) {
        clear_line();
        printf("Connection closed\n");
    } else {
        printf("Send: %s", buffer);
    }
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
        fprintf(stderr, "Authentication failed");
        fprintf(stderr, "%s", message.text);
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
                handle_history_command(socket, username);
            } else if (strncmp(buffer, "/list", 5) == 0) {
                handle_list_command(socket, username);
            } else if (strncmp(buffer, "/quit", 5) == 0) {
                handle_quit_command(socket, username);
                break;
            } else if (strncmp(buffer, "/msg", 4) == 0) {
                handle_msg_command(socket, username, buffer);
            } else {
                handle_regular_message(socket, username, buffer);
            }
        }

        if (FD_ISSET(socket, &readfds)) {
            if (receive_message(socket) != RET_OK) {
                break;
            }
        }
    }

    close(socket);

    exit(EXIT_SUCCESS);
}
