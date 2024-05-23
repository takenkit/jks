#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#define MAX_BUFFER_SIZE 1024
#define MAX_QUEUE_SIZE  100
#define RET_OK          0
#define RET_NG          -1

#define COMMAND_QUIT    1
#define COMMAND_LIST    2
#define COMMAND_HISTORY 3
#define COMMAND_MESSAGE 4
#define COMMAND_NONE    0

#define MAX_IP_LENGTH         16
#define MAX_USERNAME_LENGTH   256
#define MAX_MESSAGE_LENGTH    16384 
#define MAX_COMMAND_LENGTH    64

#define MAX_CLIENTS             256 
#define CHAT_HISTORY_FILENAME   "chat_history.txt"

typedef struct {
    unsigned int sequence;                  // シーケンス番号
    unsigned int from_port;                 // 送信元ポート番号
    unsigned int to_port;                   // 宛先ポート番号
    unsigned short year;                    // 年
    unsigned char month;                    // 月
    unsigned char day;                      // 日
    unsigned char hour;                     // 時
    unsigned char minute;                   // 分
    unsigned char second;                   // 秒
    unsigned char millisecond;              // 10ミリ秒
    unsigned char from_ip[MAX_IP_LENGTH];   // 送信元IPアドレス
    unsigned char to_ip[MAX_IP_LENGTH];     // 宛先IPアドレス
    char from_user[MAX_USERNAME_LENGTH];    // 送信元ユーザ名
    char to_user[MAX_USERNAME_LENGTH];      // 宛先ユーザ名
    char message[MAX_MESSAGE_LENGTH];       // メッセージ
    char command[MAX_COMMAND_LENGTH];       // コマンド
} ChatMessage;

int create_server_socket(int port, int *server_sock);
int create_client_socket(const char *server_addr, int port, int *client_sock);
int send_data(int sock, const void *data, int length);
int recv_data(int sock, void *buffer, int buffer_size);