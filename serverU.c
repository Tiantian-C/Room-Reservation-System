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

#define LOCAL_HOST "127.0.0.1" // Host address
#define ServerU_UDP_PORT 43074 // Server U port number
#define DEST_PORT 44074        // Main Server port number
#define FAIL -1
#define MAX_ROOMS 100
#define MAXDATASIZE 1024
#define MAX_ROOMCODE_SIZE 15
#define MAX_OPTION_SIZE 20

typedef struct
{
    char roomCode[10];
    int available;
} Room;

int sockfd_serverU_UDP;                     // Server U datagram socket
struct sockaddr_in serverU_addr, main_addr; // main address as a server & as a client
Room roomData[MAX_ROOMS];                   // Store room data
int roomCount = 0;

char recv_buf[MAXDATASIZE];
char roomcode[MAX_ROOMCODE_SIZE];
char option[MAX_OPTION_SIZE];

// /**
//  * Defined functions
//  */

// 1. Create UDP socket
void create_serverU_socket();

// 2. Initialize connection with Main server
void init_Main_connection();

// 3. Bind a socket
void bind_socket();

// 4. Loading the single.txt
void loadRoomData(const char *filename);

void create_serverU_socket()
{
    sockfd_serverU_UDP = socket(AF_INET, SOCK_DGRAM, 0); // Create a UDP socket
    if (sockfd_serverU_UDP == FAIL)
    {
        perror("[ERROR] server U: can not open socket");
        exit(1);
    }
}

void init_Main_connection()
{

    // Server U side information
    // Initialize server S IP address, port number
    memset(&serverU_addr, 0, sizeof(serverU_addr));       //  make sure the struct is empty
    serverU_addr.sin_family = AF_INET;                    // Use IPv4 address family
    serverU_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST); // Host IP address
    serverU_addr.sin_port = htons(ServerU_UDP_PORT);      // Server S port number

    memset(&main_addr, 0, sizeof(main_addr)); // 确保结构体的其余部分被清零
    main_addr.sin_family = AF_INET;
    main_addr.sin_port = htons(DEST_PORT);             // 替换DEST_PORT为目标端口号
    main_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST); // LOCAL_HOST已经定义为"127.0.0.1"
}

/**
 * Bind socket with specified IP address and port number
 */
void bind_socket()
{
    if (bind(sockfd_serverU_UDP, (struct sockaddr *)&serverU_addr, sizeof(serverU_addr)) == FAIL)
    {
        perror("[ERROR] Server U: fail to bind Server U UDP socket");
        exit(1);
    }

    printf("The Server U is up and running using UDP on port %d. \n", ServerU_UDP_PORT);
}

void loadRoomData(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Unable to open the file");
        exit(EXIT_FAILURE);
    }

    while (fscanf(file, "%[^,], %d\n", roomData[roomCount].roomCode, &roomData[roomCount].available) == 2)
    {
        roomCount++;
        if (roomCount >= MAX_ROOMS)
            break; // 避免溢出
    }

    fclose(file);
}

int main()
{

    /******    Step 0: Load and store the data from the file ******/
    loadRoomData("suite.txt");
    /******    Step 1: Create server U socket (UDP)  ******/
    create_serverU_socket();
    /******    Step 2: Create sockaddr_in struct  ******/
    init_Main_connection();
    /******    Step 3: Bind socket with specified IP address and port number  ******/
    bind_socket();

    socklen_t main_addr_size = sizeof(main_addr);

    char message[MAXDATASIZE]; // All the room status of Server S

    for (int i = 0; i < roomCount; i++)
    {
        char roomStatus[100]; // one room staus
        sprintf(roomStatus, "%s, %d\n", roomData[i].roomCode, roomData[i].available);
        strcat(message, roomStatus);
    }

    if (sendto(sockfd_serverU_UDP, message, sizeof(message) + 1, 0, (struct sockaddr *)&main_addr, main_addr_size) < 0)
    {
        perror("[ERROR] Server U: fail to send write result to Main server");
        exit(1);
    }
    printf("The Server U has sent the room status to the main server.\n");

    /*  RECEIVE Availability OR Reservation request from main server  */

    if (recvfrom(sockfd_serverU_UDP, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&main_addr,
                 &main_addr_size) == FAIL)
    {
        perror("[ERROR] Server S: fail to receive data from main server");
        exit(1);
    }

    recv_buf[MAXDATASIZE - 1] = '\0';

    // get roomcode
    char *token = strtok(recv_buf, "|");
    if (token != NULL)
    {
        strncpy(roomcode, token, MAX_ROOMCODE_SIZE - 1);
        roomcode[MAX_ROOMCODE_SIZE - 1] = '\0'; // 确保字符串结束
    }

    // get option
    token = strtok(NULL, "|");
    if (token != NULL)
    {
        strncpy(option, token, MAX_OPTION_SIZE - 1);
        option[MAX_OPTION_SIZE - 1] = '\0'; // 确保字符串结束
    }

    if (strcmp(option, "Availability") == 0)
    {
        // Availability Request
        printf("The server U received an availability request from the main server.\n");
        int roomFound = 0; // Flag to check if the room is found

        for (int i = 0; i < roomCount; i++)
        {
            if (strcmp(roomData[i].roomCode, roomcode) == 0) // Corrected condition
            {
                roomFound = 1; // Mark as found
                if (roomData[i].available > 0)
                {
                    printf("Room %s is available.\n", roomcode);
                    char *availability_message = "The requested room is available.";
                    if (sendto(sockfd_serverU_UDP, availability_message, strlen(availability_message) + 1, 0, (struct sockaddr *)&main_addr, main_addr_size) < 0)
                    {
                        perror("[ERROR] Server U: fail to send availability result to Main server");
                        exit(1);
                    }
                }
                else
                {
                    printf("Room %s is not available.\n", roomcode);
                    char *availability_message = "The requested room is not available.";
                    if (sendto(sockfd_serverU_UDP, availability_message, strlen(availability_message) + 1, 0, (struct sockaddr *)&main_addr, main_addr_size) < 0)
                    {
                        perror("[ERROR] Server U: fail to send availability result to Main server");
                        exit(1);
                    }
                }
                break; // Exit the loop after handling the found room
            }
        }

        if (!roomFound) // If no room was found
        {
            printf("Not able to find the room layout.\n");
            char *message = "Not able to find the room layout.";
            sendto(sockfd_serverU_UDP, message, strlen(message) + 1, 0, (struct sockaddr *)&main_addr, main_addr_size);
        }
        printf("The server U finished sending the response to the main server.\n");
    }
    else
    {
        // Reservation Request
        printf("The server U received a reservation request from the main server.\n");
        int roomFound = 0; // Flag to check if the room is found

        for (int i = 0; i < roomCount; i++)
        {
            if (strcmp(roomData[i].roomCode, roomcode) == 0) // Corrected condition
            {
                roomFound = 1; // Mark as found
                if (roomData[i].available > 0)
                {
                    roomData[i].available--;
                    printf("Successful reservation.The count of the Room %s is now %d.\n", roomcode, roomData[i].available);
                    char reservation_message[200];
                    sprintf(reservation_message, "Congratulations! The reservation for Room %s has been made.\n", roomcode);
                    // char *reservation_message = "Congratulation!The reservation for Room has been made.\n";
                    if (sendto(sockfd_serverU_UDP, reservation_message, strlen(reservation_message) + 1, 0, (struct sockaddr *)&main_addr, main_addr_size) < 0)
                    {
                        perror("[ERROR] Server U: fail to send availability result to Main server");
                        exit(1);
                    }
                    printf("The server U finished sending the response and updated room status to the main server.\n");
                }
                else
                {
                    printf("Cannot make a reservation.Room %s is not available.\n", roomcode);
                    char *reservation_message = "The requested room is not available.";
                    if (sendto(sockfd_serverU_UDP, reservation_message, strlen(reservation_message) + 1, 0, (struct sockaddr *)&main_addr, main_addr_size) < 0)
                    {
                        perror("[ERROR] Server S: fail to send availability result to Main server");
                        exit(1);
                    }
                    printf("The server U finished sending the response to the main server.\n");
                }
                break; // Exit the loop after handling the found room
            }
        }
        if (!roomFound) // If no room was found
        {
            printf("Cannot make a reservation.Not able to find the room layout.\n");
            char *message = "Cannot make a reservation.Not able to find the room layout.";
            sendto(sockfd_serverU_UDP, message, strlen(message) + 1, 0, (struct sockaddr *)&main_addr, main_addr_size);
            printf("The server S finished sending the response to the main server.\n");
        }
    }
}
