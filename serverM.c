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
#define Main_UDP_PORT 44074 // Main port number
#define ServerS_PORT 41074
#define ServerD_PORT 42074
#define ServerU_PORT 43074
#define MAXDATASIZE 1024 // max number of bytes we can get at once
#define BACKLOG 10 // max number of connections allowed on the incoming queue
#define FAIL -1

int sockfd_UDP; // UDP socket
struct sockaddr_in main_UDP_addr;
struct sockaddr_in dest_serverS_addr, dest_serverD_addr,dest_serverU_addr; // When AWS works as a client


char S_room_status[MAXDATASIZE]; // Initail single room status returned from server S
char D_room_status[MAXDATASIZE]; // Initail double room status returned from server D
char U_room_status[MAXDATASIZE]; // Initail suite room status returned from server U



// 1. Create UDP socket and bind socket
void create_UDP_socket();

void init_connection_serverS();

void init_connection_serverD();

void init_connection_serverU();



/**
 * Step 1: Create UDP socket and bind socket
 */
void create_UDP_socket() {
    sockfd_UDP = socket(AF_INET, SOCK_DGRAM, 0); // UDP datagram socket
    if (sockfd_UDP == FAIL) {
        perror("[ERROR] Main server: fail to create UDP socket");
        exit(1);
    }

    // Initialize Local IP address, port number
    memset(&main_UDP_addr, 0, sizeof(main_UDP_addr)); //  make sure the struct is empty
    main_UDP_addr.sin_family = AF_INET; // Use IPv4 address family
    main_UDP_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST); // Host IP address
    main_UDP_addr.sin_port = htons(Main_UDP_PORT); // Port number for client

    // Bind socket
    if (bind(sockfd_UDP, (struct sockaddr *) &main_UDP_addr, sizeof(main_UDP_addr)) == FAIL) {
        perror("[ERROR] Main server: fail to bind UDP socket");
        exit(1);
    }
}


void init_connection_serverS() {
    // Info about server S
    memset(&dest_serverS_addr, 0, sizeof(dest_serverS_addr));
    dest_serverS_addr.sin_family = AF_INET;
    dest_serverS_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    dest_serverS_addr.sin_port = htons(ServerS_PORT);
}

void init_connection_serverD() {
    // Info about server D
    memset(&dest_serverD_addr, 0, sizeof(dest_serverD_addr));
    dest_serverD_addr.sin_family = AF_INET;
    dest_serverD_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    dest_serverD_addr.sin_port = htons(ServerD_PORT);
}

void init_connection_serverU() {
    // Info about server U
    memset(&dest_serverU_addr, 0, sizeof(dest_serverU_addr));
    dest_serverU_addr.sin_family = AF_INET;
    dest_serverU_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    dest_serverU_addr.sin_port = htons(ServerD_PORT);
}


int main(){
    // Create and bind the UDP socket
    create_UDP_socket();
    printf("The main server is up and running.\n");

    init_connection_serverS();
    // Receive from Server S: Initail roomstatus from S
    socklen_t dest_serverS_size = sizeof(dest_serverS_addr);
    if (recvfrom(sockfd_UDP, S_room_status, 1, 0, (struct sockaddr *) &dest_serverS_addr, &dest_serverS_size) == FAIL) {
        perror("[ERROR] AWS: fail to receive writing result from Server S");
        exit(1);
    }
    printf("The main server has received the room status from Server S using UDP over port %d\n",Main_UDP_PORT);

    init_connection_serverD();
    // Receive from Server D: Initail roomstatus from D
    socklen_t dest_serverD_size = sizeof(dest_serverD_addr);
    if (recvfrom(sockfd_UDP, D_room_status, 1, 0, (struct sockaddr *) &dest_serverD_addr, &dest_serverD_size) == FAIL) {
        perror("[ERROR] Main: fail to receive writing result from Server D");
        exit(1);
    }
    printf("The main server has received the room status from Server D using UDP over port %d\n",Main_UDP_PORT);

    init_connection_serverU();
    // Receive from Server U: Initail roomstatus from U
    socklen_t dest_serverU_size = sizeof(dest_serverU_addr);
    if (recvfrom(sockfd_UDP, U_room_status, 1, 0, (struct sockaddr *) &dest_serverU_addr, &dest_serverU_size) == FAIL) {
        perror("[ERROR] Main: fail to receive writing result from Server U");
        exit(1);
    }
    printf("The main server has received the room status from Server U using UDP over port %d\n",Main_UDP_PORT);
}

