#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#define LOCAL_HOST "127.0.0.1" // Host address
#define Main_UDP_PORT 44074    // Main UDP port number
#define Main_TCP_PORT 45074    // Main TCP port number

#define ServerS_PORT 41074
#define ServerD_PORT 42074
#define ServerU_PORT 43074
#define MAXDATASIZE 1024 // max number of bytes we can get at once
#define BACKLOG 10       // max number of connections allowed on the incoming queue
#define FAIL -1

#define MAX_USERS 100 // at most 100 users
#define MAX_NAME_LENGTH 50
#define MAX_PASS_LENGTH 50

// 用户结构体定义
typedef struct
{
    char username[MAX_NAME_LENGTH];
    char password[MAX_PASS_LENGTH];
} User;

// 用户数组和用户计数器
User users[MAX_USERS];
int userCount = 0;

int sockfd_UDP;   // UDP socket
int sockfd_TCP;   // TCP parent socket
int child_sockfd; // TCP child socket

struct sockaddr_in main_UDP_addr, main_member_addr, main_guest_addr;
struct sockaddr_in dest_serverS_addr, dest_serverD_addr, dest_serverU_addr, dest_member_addr; // When Main server works as a client

char S_room_status[MAXDATASIZE]; // Initail single room status returned from server S
char D_room_status[MAXDATASIZE]; // Initail double room status returned from server D
char U_room_status[MAXDATASIZE]; // Initail suite room status returned from server U

char login_buf[MAXDATASIZE]; // login data from client

void readMembersFromFile(const char *filename);
// 1. Create UDP socket and bind socket
void create_UDP_socket();

// 2. Create TCP socket w/ member & bind socket
void create_TCP_socket();

// 3. Listen for clients
void listen_client_one();

void init_connection_serverS();

void init_connection_serverD();

void init_connection_serverU();

void readMembersFromFile(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Unable to open the file");
        exit(EXIT_FAILURE);
    }

    while (fscanf(file, "%[^,], %s\n", users[userCount].username, users[userCount].password) == 2)
    {
        userCount++;
        if (userCount >= MAX_USERS)
            break;
    }

    fclose(file);
}
/**
 * Step 1: Create UDP socket and bind socket
 */
void create_UDP_socket()
{
    sockfd_UDP = socket(AF_INET, SOCK_DGRAM, 0); // UDP datagram socket
    if (sockfd_UDP == FAIL)
    {
        perror("[ERROR] Main server: fail to create UDP socket");
        exit(1);
    }

    // Initialize Local IP address, port number
    memset(&main_UDP_addr, 0, sizeof(main_UDP_addr));      //  make sure the struct is empty
    main_UDP_addr.sin_family = AF_INET;                    // Use IPv4 address family
    main_UDP_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST); // Host IP address
    main_UDP_addr.sin_port = htons(Main_UDP_PORT);         // Port number for client

    // Bind socket
    if (bind(sockfd_UDP, (struct sockaddr *)&main_UDP_addr, sizeof(main_UDP_addr)) == FAIL)
    {
        perror("[ERROR] Main server: fail to bind UDP socket");
        exit(1);
    }
}

/**
 * Step 2: Create TCP socket for member client & bind socket
 */
void create_TCP_socket()
{
    sockfd_TCP = socket(AF_INET, SOCK_STREAM, 0); // Create TCP socket
    if (sockfd_TCP == FAIL)
    {
        perror("[ERROR] Main server: fail to create socket for client");
        exit(1);
    }

    // Initialize IP address, port number
    memset(&main_member_addr, 0, sizeof(main_member_addr));   //  make sure the struct is empty
    main_member_addr.sin_family = AF_INET;                    // Use IPv4 address family
    main_member_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST); // Host IP address
    main_member_addr.sin_port = htons(Main_TCP_PORT);         // Port number for client

    // Bind socket for client with IP address and port number for client
    if (bind(sockfd_TCP, (struct sockaddr *)&main_member_addr, sizeof(main_member_addr)) == FAIL)
    {
        perror("[ERROR] Main server: fail to bind client socket");
        exit(1);
    }
}

/**
 * Step 3: Listen for incoming connection from clients
 */
void listen_client()
{
    if (listen(sockfd_TCP, BACKLOG) == FAIL)
    {
        perror("[ERROR] AWS server: fail to listen for client socket");
        exit(1);
    }
}

void init_connection_serverS()
{
    // Info about server S
    memset(&dest_serverS_addr, 0, sizeof(dest_serverS_addr));
    dest_serverS_addr.sin_family = AF_INET;
    dest_serverS_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    dest_serverS_addr.sin_port = htons(ServerS_PORT);
}

void init_connection_serverD()
{
    // Info about server D
    memset(&dest_serverD_addr, 0, sizeof(dest_serverD_addr));
    dest_serverD_addr.sin_family = AF_INET;
    dest_serverD_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    dest_serverD_addr.sin_port = htons(ServerD_PORT);
}

void init_connection_serverU()
{
    // Info about server U
    memset(&dest_serverU_addr, 0, sizeof(dest_serverU_addr));
    dest_serverU_addr.sin_family = AF_INET;
    dest_serverU_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    dest_serverU_addr.sin_port = htons(ServerD_PORT);
}

// int validateUser(const char *username, const char *password)
// {
//     for (int i = 0; i < userCount; i++)
//     {
//         if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0)
//         {
//             return 1; // login success
//         }
//         else if (strcmp(users[i].username, username) != 0 && strcmp(users[i].password, password) == 0)
//         {
//             return 2;
//         }
//         else if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) != 0)
//         {
//             return 3;
//         }
//     }
//     return 0; // login failed
// }

int validateUser(const char *username, const char *password)
{
    int usernameFound = 0; // 用于标记是否找到用户名

    for (int i = 0; i < userCount; i++)
    {
        if (strcmp(users[i].username, username) == 0)
        {
            usernameFound = 1; // 找到了用户名
            if (strcmp(users[i].password, password) == 0)
            {
                return 1; // 登录成功
            }
            else
            {
                return 3; // 用户名匹配，但密码不匹配
            }
        }
    }

    if (!usernameFound)
    {
        return 2; // 没有找到用户名
    }

    return 0; // 默认返回登录失败（这个情况应该不会发生，因为已经覆盖了所有可能）
}

int main()
{

    readMembersFromFile("member.txt"); // 调用函数读取文件

    // Create TCP socket & bind socket
    create_TCP_socket();
    listen_client();

    // Create and bind the UDP socket
    create_UDP_socket();
    init_connection_serverS();
    init_connection_serverD();
    init_connection_serverU();

    printf("The main server is up and running.\n");

    /**********      Receive initial Status from Server S,D,U     **********/

    // Receive from Server S: Initail roomstatus from S
    socklen_t dest_serverS_size = sizeof(dest_serverS_addr);
    if (recvfrom(sockfd_UDP, S_room_status, 1, 0, (struct sockaddr *)&dest_serverS_addr, &dest_serverS_size) == FAIL)
    {
        perror("[ERROR] AWS: fail to receive writing result from Server S.");
        exit(1);
    }
    printf("The main server has received the room status from Server S using UDP over port %d.\n", Main_UDP_PORT);

    // Receive from Server D: Initail roomstatus from D
    socklen_t dest_serverD_size = sizeof(dest_serverD_addr);
    if (recvfrom(sockfd_UDP, D_room_status, 1, 0, (struct sockaddr *)&dest_serverD_addr, &dest_serverD_size) == FAIL)
    {
        perror("[ERROR] Main: fail to receive writing result from Server D.");
        exit(1);
    }
    printf("The main server has received the room status from Server D using UDP over port %d.\n", Main_UDP_PORT);

    // Receive from Server U: Initail roomstatus from U
    socklen_t dest_serverU_size = sizeof(dest_serverU_addr);
    if (recvfrom(sockfd_UDP, U_room_status, 1, 0, (struct sockaddr *)&dest_serverU_addr, &dest_serverU_size) == FAIL)
    {
        perror("[ERROR] Main: fail to receive writing result from Server U.");
        exit(1);
    }
    printf("The main server has received the room status from Server U using UDP over port %d.\n", Main_UDP_PORT);

    struct sockaddr_in their_addr; // client's address
    socklen_t sin_size = sizeof(their_addr);

    while (1)
    { // main accept loop
        child_sockfd = accept(sockfd_TCP, (struct sockaddr *)&their_addr, &sin_size);
        if (child_sockfd == -1)
        {
            perror("accept");
            continue;
        }

        /*********************     process the connection         ********************************/

        if (!fork())
        {
            // This is child process
            close(sockfd_TCP); // child process don't need lisener
            // a connection with client
            // 使用send(), recv()等函数与客户端进行通信
            int recv_client = recv(child_sockfd, login_buf, MAXDATASIZE, 0);
            if (recv_client == FAIL)
            {
                perror("[ERROR] Main server: fail to receive login data from client.");
                exit(1);
            }

            char type[10]; // 用户类型：member 或 guest
            char unencrypted_username[MAX_NAME_LENGTH];
            char username[MAX_NAME_LENGTH];
            char password[MAX_PASS_LENGTH];
            // sscanf(login_buf, "%49[^,],%49s", username, password);
            int numItems = sscanf(login_buf, "%[^,],%[^,],%[^,],%s", type, unencrypted_username, username, password);

            char welcomeMessage[1024];

            if (strcmp(type, "guest") == 0)
            {
                // 访客逻辑处理
                printf("The main server has received the guest request for %s using TCP over port %d.\n", unencrypted_username, Main_TCP_PORT);

                // 格式化欢迎消息，将未加密的用户名插入到消息中
                snprintf(welcomeMessage, sizeof(welcomeMessage), "Welcome guest %s!", unencrypted_username);
                send(child_sockfd, welcomeMessage, strlen(welcomeMessage), 0);
                printf("The main server sent guest response to the client.\n");
            }
            else if (strcmp(type, "member") == 0 && numItems >= 3)
            {
                // 成员逻辑处理
                printf("The main server has received the authentication for %s using TCP over port %d.\n", unencrypted_username, Main_TCP_PORT);

                // 对于成员，我们有未加密的用户名、加密的用户名和加密的密码
                if (validateUser(username, password) == 1)
                {
                    // if login success
                    snprintf(welcomeMessage, sizeof(welcomeMessage), "Welcome member %s!", unencrypted_username);

                    send(child_sockfd, welcomeMessage, strlen(welcomeMessage), 0);
                    printf("The main server sent authentication result to the client.\n");
                }
                else if (validateUser(username, password) == 2)
                {
                    // Username does not exist
                    char *message = "Failed login: Username does not exist.";
                    send(child_sockfd, message, strlen(message) + 1, 0);
                    printf("The main server sent authentication result to the client.\n");
                }
                else if (validateUser(username, password) == 3)
                {
                    char *message = "Failed login: Password does not match.";
                    send(child_sockfd, message, strlen(message) + 1, 0);
                    printf("The main server sent authentication result to the client.\n");
                }
            }

            close(child_sockfd); // 处理完客户端请求后关闭连接
            exit(0);
        }
        close(child_sockfd); // 父进程关闭已接受的连接
    }
}
