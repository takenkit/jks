#include "common.h"

static int client_sockets[MAX_CLIENTS];
static char connected_users[MAX_CLIENTS][MAX_USERNAME_LENGTH];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t user_list_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t chat_history_mutex = PTHREAD_MUTEX_INITIALIZER;

void add_client_socket(int client_socket)
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (client_sockets[i] == 0) {
            client_sockets[i] = client_socket;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void remove_client_socket(int client_socket)
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (client_sockets[i] == client_socket) {
            client_sockets[i] = 0;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void broadcast_message(const ChatMessage *message, int sender_socket)
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (client_sockets[i] != 0 && client_sockets[i] != sender_socket) {
            send_data(client_sockets[i], message, sizeof(ChatMessage));
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

int parse_command(const char *input)
{
    if (strncmp(input, "quit", 5) == 0) {
        return COMMAND_QUIT;
    } else if (strncmp(input, "list", 5) == 0) {
        return COMMAND_LIST;
    } else if (strncmp(input, "history", 8) == 0) {
        return COMMAND_HISTORY;
    } else if (strncmp(input, "history_end", 12) == 0) {
        return COMMAND_HISTORY_END;
    } else if (strncmp(input, "message", 8) == 0) {
        return COMMAND_MESSAGE;
    } else if (strncmp(input, "private", 8) == 0) {
        return COMMAND_PRIVATE;
    } else if (strncmp(input, "auth", 5) == 0) {
        return COMMAND_AUTH;
    } else if (strncmp(input, "auth_ok", 8) == 0) {
        return COMMAND_AUTH_OK;
    } else if (strncmp(input, "info", 5) == 0) {
        return COMMAND_INFO;
    } else if (strncmp(input, "error", 6) == 0) {
        return COMMAND_ERROR;
    } else {
        return COMMAND_NONE;
    }
}

void add_user_to_list(const char *username)
{
    pthread_mutex_lock(&user_list_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (connected_users[i][0] == '\0') {
            strncpy(connected_users[i], username, MAX_USERNAME_LENGTH - 1);
            connected_users[i][MAX_USERNAME_LENGTH - 1] = '\0';
            break;
        }
    }
    pthread_mutex_unlock(&user_list_mutex);
}

void remove_user_from_list(const char *username)
{
    pthread_mutex_lock(&user_list_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (strcmp(connected_users[i], username) == 0) {
            connected_users[i][0] = '\0';
            break;
        }
    }
    pthread_mutex_unlock(&user_list_mutex);
}

int is_username_duplicate(const char *username)
{
    pthread_mutex_lock(&user_list_mutex);
    int duplicate = 0;
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (strcmp(connected_users[i], username) == 0) {
            duplicate = 1;
            break;
        }
    }
    pthread_mutex_unlock(&user_list_mutex);
    return duplicate;
}

void send_user_list(int client_socket)
{
    ChatMessage message;
    memset(&message, 0, sizeof(ChatMessage));
    strcpy(message.command, "list");

    pthread_mutex_lock(&user_list_mutex);
    strcpy(message.text, "Connected users:\n");
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (connected_users[i][0] != '\0') {
            strcat(message.text, "- ");
            strcat(message.text, connected_users[i]);
            strcat(message.text, "\n");
        }
    }
    pthread_mutex_unlock(&user_list_mutex);

    send_data(client_socket, &message, sizeof(ChatMessage));
}

void write_message_to_history(const ChatMessage *message)
{
    pthread_mutex_lock(&chat_history_mutex);
    FILE *file = fopen(CHAT_HISTORY_FILENAME, "a");
    if (file == NULL) {
        perror("fopen() failed");
        pthread_mutex_unlock(&chat_history_mutex);
        return;
    }

    if (strcmp(message->command, "message") == 0) {
        fprintf(file, "[%04d-%02d-%02d %02d:%02d:%02d] <%s>: message: %s",
                message->year, message->month, message->day,
                message->hour, message->minute, message->second,
                message->from_user, message->text);
    } else if (strcmp(message->command, "private") == 0) {
        fprintf(file, "[%04d-%02d-%02d %02d:%02d:%02d] [Private] <%s> to <%s>: %s",
                message->year, message->month, message->day,
                message->hour, message->minute, message->second,
                message->from_user, message->to_user, message->text);
    } else if (strcmp(message->command, "info") == 0) {
        fprintf(file, "[%04d-%02d-%02d %02d:%02d:%02d] %s",
                message->year, message->month, message->day,
                message->hour, message->minute, message->second,
                message->text);
    }
    
    fclose(file);
    pthread_mutex_unlock(&chat_history_mutex);
}

void send_message_history(int client_socket)
{
    pthread_mutex_lock(&chat_history_mutex);
    FILE *file = fopen(CHAT_HISTORY_FILENAME, "r");
    if (file == NULL) {
        perror("fopen() failed");
        ChatMessage message;
        memset(&message, 0, sizeof(ChatMessage));
        strcpy(message.command, "error");
        strcpy(message.text, "Failed to open history file\n");
        send_data(client_socket, &message, sizeof(ChatMessage));
        pthread_mutex_unlock(&chat_history_mutex);
        return;
    }

    ChatMessage message;
    memset(&message, 0, sizeof(ChatMessage));
    strcpy(message.command, "history");

    char buffer[MAX_BUFFER_SIZE];
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        strcat(message.text, buffer);
        if (strlen(message.text) + sizeof(buffer) > sizeof(message.text)) {
            send_data(client_socket, &message, sizeof(ChatMessage));
            memset(message.text, 0, sizeof(message.text));
        }
    }

    if (strlen(message.text) > 0) {
        send_data(client_socket, &message, sizeof(ChatMessage));
    }

    // 履歴データの終了を示すメッセージを送信
    memset(&message, 0, sizeof(ChatMessage));
    strcpy(message.command, "history_end");
    send_data(client_socket, &message, sizeof(ChatMessage));

    fclose(file);
    pthread_mutex_unlock(&chat_history_mutex);
}

void send_private_message(const ChatMessage *message)
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (strcmp(connected_users[i], message->to_user) == 0) {
            send_data(client_sockets[i], message, sizeof(ChatMessage));
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg)
{
    int client_socket = *(int *)arg;
    ChatMessage message;
    char username[MAX_USERNAME_LENGTH];
    int recv_size;

    memset(&message, 0, sizeof(ChatMessage));

    // ユーザー名の受信
    recv_size = recv_data(client_socket, &message, sizeof(ChatMessage));
    if (recv_size <= 0) {
        perror("recv_data() failed");
        close(client_socket);
        return NULL;
    }
    strncpy(username, message.from_user, sizeof(username) - 1);

    // ユーザー名の重複チェック
    if (is_username_duplicate(username)) {
        strcpy(message.command, "error");
        strcpy(message.text, "Username already taken. Please choose another one.\n");
        send_data(client_socket, &message, sizeof(ChatMessage));
        close(client_socket);
        return NULL;
    }

    // ユーザーリストに追加
    add_user_to_list(username);

    // 認証成功メッセージの送信
    strcpy(message.command, "auth_ok");
    sprintf(message.text, "Hello %s!\n", username);
    send_data(client_socket, &message, sizeof(ChatMessage));

    strcpy(message.command, "info");
    sprintf(message.text, "%s logined in\n", username);
    write_message_to_history(&message);
    broadcast_message(&message, client_socket);

    // メインループでメッセージの送受信を処理
    while ((recv_size = recv_data(client_socket, &message, sizeof(ChatMessage))) > 0) {
        if ((unsigned int)recv_size < sizeof(ChatMessage)) {
            perror("recv_data() incomplete message");
            break;
        }

        int command = parse_command(message.command);
        if (command == COMMAND_QUIT) {
            break;
        } else if (command == COMMAND_LIST) {
            send_user_list(client_socket);
        } else if (command == COMMAND_HISTORY) {
            send_message_history(client_socket);
        } else if (command == COMMAND_PRIVATE) {
            write_message_to_history(&message);
            send_private_message(&message);
        } else if (command != COMMAND_NONE) {
            write_message_to_history(&message);
            broadcast_message(&message, client_socket);
        }
    }

    if (recv_size < 0) {
        perror("recv_data() error");
    }
    
    strcpy(message.command, "info");
    sprintf(message.text, "%s logout\n", message.from_user);
    write_message_to_history(&message);
    broadcast_message(&message, client_socket);

    // ユーザーリストから削除
    remove_user_from_list(username);

    close(client_socket);
    remove_client_socket(client_socket);

    return NULL;
}

int main(int argc, char *argv[])
{
    int server_socket, new_socket, max_sd, sd;
    fd_set readfds;
    struct sockaddr_in address;
    socklen_t length = sizeof(address);

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP address> <Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[2]);

    if (create_server_socket(port, &server_socket) != 0) {
        perror("create_server_socket() failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);
        max_sd = server_socket;

        for (int i = 0; i < MAX_CLIENTS; ++i) {
            sd = client_sockets[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            perror("select() failed");
        }

        if (FD_ISSET(server_socket, &readfds)) {
            if ((new_socket = accept(server_socket, (struct sockaddr *)&address, &length)) < 0) {
                perror("accept() failed");
                exit(EXIT_FAILURE);
            }

            add_client_socket(new_socket);

            pthread_t thread_id;
            pthread_create(&thread_id, NULL, handle_client, &new_socket);
            pthread_detach(thread_id);
        }
    }

    close(server_socket);

    exit(EXIT_SUCCESS);
}
