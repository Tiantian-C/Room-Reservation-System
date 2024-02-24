#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>

#include <arpa/inet.h>

/**
 * Named Constants
 */
#define LOCAL_HOST "127.0.0.1"     // Host address
#define Main_Client_TCP_PORT 45074 // Main port number
#define MAXDATASIZE 1024           // max number of bytes we can get at once
#define FAIL -1                    // socket fails if result = -1

/**
 * Defined global variables
 */
int sockfd_client_TCP;          // Client socket
struct sockaddr_in main_addr;   // Main server address
char userdata_buf[MAXDATASIZE]; // Store input to userdata (send to Main)
// char compute_buf[MAXDATASIZE]; // Store input to compute (send to Main)
char login_result[MAXDATASIZE];   // login result from Main
char compute_result[MAXDATASIZE]; // Compute result from Main

/**
 * Steps (defined functions):
 */
// 1. Create TCP socket
void create_client_socket_TCP();

// 2. Initialize TCP connection with AWS
void init_Main_connection();

// 3. Send connection request to AWS server
void request_Main_connection();

// 4. Send data to AWS (write / compute)

// 5. Get result back from AWS Server (write result / computed result)

/**
 * Step 1: Create client socket (TCP stream socket)
 */
void create_client_socket_TCP()
{
    sockfd_client_TCP = socket(AF_INET, SOCK_STREAM, 0); // Create a TCP socket
    if (sockfd_client_TCP == FAIL)
    {
        perror("[ERROR] client: can not open client socket ");
        exit(1);
    }
}

/**
 * Step 2: Initial TCP connection info
 */
void init_Main_connection()
{

    // *** Beej’s guide to network programming - 9.24. struct sockaddr and pals ***
    // Initialize TCP connection between client and Main server using specified IP address and port number
    memset(&main_addr, 0, sizeof(main_addr));          //  make sure the struct is empty
    main_addr.sin_family = AF_INET;                    // Use IPv4 address family
    main_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST); // Source address
    main_addr.sin_port = htons(Main_Client_TCP_PORT);  // Main server port number
}

/**
 * Step 3: Send connection request to AWS server
 */
void request_Main_connection()
{
    connect(sockfd_client_TCP, (struct sockaddr *)&main_addr, sizeof(main_addr));

    // If connection succeed, display boot up message
    printf("The client is up and running \n");
}

void encrypt(char *input)
{
    for (int i = 0; input[i] != '\0'; i++)
    {
        if (input[i] >= 'a' && input[i] <= 'z')
        { // Lowercase letters
            input[i] = (input[i] - 'a' + 3) % 26 + 'a';
        }
        else if (input[i] >= 'A' && input[i] <= 'Z')
        { // Uppercase letters
            input[i] = (input[i] - 'A' + 3) % 26 + 'A';
        }
        else if (input[i] >= '0' && input[i] <= '9')
        { // Digits
            input[i] = (input[i] - '0' + 3) % 10 + '0';
        }
        // Special characters are not changed
    }
}

int main()
{

    create_client_socket_TCP();
    init_Main_connection();
    request_Main_connection();

    char username[51];      // store username
    char password[51];      // store password
    char username_copy[51]; // 用户名副本的缓冲区
    int isMember = 0;

    printf("Please enter the username: ");
    scanf("%50s", username); // 读取用户输入的用户名，使用50来限制输入长度，防止溢出

    strcpy(username_copy, username); // 复制username到username_copy

    printf("Please enter the password (Press 'Enter' to skip): ");
    getchar();                                // 捕获之前留下的换行符
    fgets(password, sizeof(password), stdin); // 使用fgets读取行，以便捕获回车

    if (strcmp(password, "\n") == 0)
    {
        // 用户按了回车，没有输入密码，视为访客
        snprintf(userdata_buf, sizeof(userdata_buf), "guest,%s", username); // 格式化消息
    }
    else
    {
        // 用户输入了密码，视为成员
        isMember = 1;                          // 标记为member
        password[strcspn(password, "\n")] = 0; // 移除密码字符串末尾的换行符
        encrypt(username);
        encrypt(password);
        snprintf(userdata_buf, sizeof(userdata_buf), "member,%s,%s,%s", username_copy, username, password); // 格式化消息,store username and password
    }

    // printf("%s", userdata_buf);

    /******    Step 4:  Send user data to Main server(member/guest)    *******/
    if (send(sockfd_client_TCP, userdata_buf, sizeof(userdata_buf), 0) == FAIL)
    {
        perror("[ERROR] client: fail to send input data");
        close(sockfd_client_TCP);
        exit(1);
    }
    if (isMember)
    {
        printf("%s sent an authentication request to the main server.\n", username_copy);
    }
    else
    {
        struct sockaddr_in localAddr;
        socklen_t addrLength = sizeof(localAddr);
        if (getsockname(sockfd_client_TCP, (struct sockaddr *)&localAddr, &addrLength) < 0)
        {
            perror("getsockname() failed");
            exit(EXIT_FAILURE);
        }

        // 将端口号从网络字节顺序转换为主机字节顺序，然后打印
        printf("%s sent a guest request to the main server using TCP over port %d.\n", username_copy, ntohs(localAddr.sin_port));
    }

    /******    Step 5:  Get login result back from main Server    *******/
    int bytes_received = recv(sockfd_client_TCP, login_result, sizeof(login_result), 0);
    if (bytes_received == FAIL)
    {
        perror("[ERROR] client: fail to receive login result from main server");
        close(sockfd_client_TCP);
        exit(1);
    }

    // 确保接收到的数据是以空字符结尾的
    login_result[bytes_received] = '\0';

    printf("%s\n", login_result);

    // 比较接收到的消息
    // if (isMember)
    // {
    //     // Member Logic
    //     if (strcmp(login_result, "Success") == 0)
    //     {
    //         printf("Welcome member %s!\n", username_copy);
    //     }
    //     else
    //     {
    //         printf("%s\n", login_result); // 打印接收到的消息
    //     }
    // }else{
    //     // Guest Logic
    //     print
    // }
}
