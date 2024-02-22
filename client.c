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
#define LOCAL_HOST "127.0.0.1" // Host address
#define Main_Client_TCP_PORT 44074 // Main port number
#define MAXDATASIZE 1024 // max number of bytes we can get at once
#define FAIL -1 // socket fails if result = -1

/**
 * Defined global variables
 */
int sockfd_client_TCP; // Client socket
struct sockaddr_in main_addr; // Main server address
char userdata_buf[MAXDATASIZE]; // Store input to userdata (send to Main)
char compute_buf[MAXDATASIZE]; // Store input to compute (send to Main)
char write_result[MAXDATASIZE]; // Write result from Main
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
void create_client_socket_TCP() {
    sockfd_client_TCP = socket(AF_INET, SOCK_STREAM, 0); // Create a TCP socket
    if (sockfd_client_TCP == FAIL) {
        perror("[ERROR] client: can not open client socket ");
        exit(1);
    }

}

/**
 * Step 2: Initial TCP connection info
 */
void init_Main_connection() {

    // *** Beej’s guide to network programming - 9.24. struct sockaddr and pals ***
    // Initialize TCP connection between client and AWS server using specified IP address and port number
    memset(&main_addr, 0, sizeof(main_addr)); //  make sure the struct is empty
    main_addr.sin_family = AF_INET; // Use IPv4 address family
    main_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST); // Source address
    main_addr.sin_port = htons(Main_Client_TCP_PORT); // Main server port number
}


/**
 * Step 3: Send connection request to AWS server
 */
void request_Main_connection() {
    connect(sockfd_client_TCP, (struct sockaddr *) &main_addr, sizeof(main_addr));

    

    // If connection succeed, display boot up message
    printf("The client is up and running \n");
}


void encrypt(char *input) {
    for (int i = 0; input[i] != '\0'; i++) {
        if (input[i] >= 'a' && input[i] <= 'z') { // Lowercase letters
            input[i] = (input[i] - 'a' + 3) % 26 + 'a';
        } else if (input[i] >= 'A' && input[i] <= 'Z') { // Uppercase letters
            input[i] = (input[i] - 'A' + 3) % 26 + 'A';
        } else if (input[i] >= '0' && input[i] <= '9') { // Digits
            input[i] = (input[i] - '0' + 3) % 10 + '0';
        }
        // Special characters are not changed
    }
}


int main(){

    create_client_socket_TCP();
    init_Main_connection();
    request_Main_connection();

    char username[51]; // store username
    char password[51]; // store password

    
    printf("Please enter the username: ");
    scanf("%50s", username); // 读取用户输入的用户名，使用50来限制输入长度，防止溢出
    printf("Please enter the password: ");
    scanf("%50s", password); // 读取用户输入的密码，同样使用50来限制输入长度

    encrypt(username);
    encrypt(password);

    snprintf(userdata_buf, sizeof(userdata_buf), "%s,%s", username, password);
    


    /******    Step 4:  Send encrpted user data to Main server    *******/
    if (send(sockfd_client_TCP,userdata_buf, sizeof(userdata_buf), 0) == FAIL) {
        perror("[ERROR] client: fail to send input data");
        close(sockfd_client_TCP);
        exit(1);
    }
    printf("The client sent write operation to AWS \n");

    // /******    Step 5:  Get write result back from AWS Server    *******/
    // if (recv(sockfd_client_TCP, write_result, sizeof(write_result), 0) == FAIL) {
    //     perror("[ERROR] client: fail to receive write result from AWS server");
    //     close(sockfd_client_TCP);
    //     exit(1);
    // }

}

