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
#define ServerS_UDP_PORT 41074 // Server U port number
#define DEST_PORT 44074 // Main Server port number
#define FAIL -1 
#define MAX_ROOMS 100
#define MAXDATASIZE 1024

typedef struct {
    char roomCode[10];
    int available;
} Room;

int sockfd_serverU_UDP; // Server U datagram socket
struct sockaddr_in serverU_addr, main_addr; // main address as a server & as a client
Room roomData[MAX_ROOMS]; // Store room data
int roomCount = 0;

char recv_buf[MAXDATASIZE]; 



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
void loadRoomData(const char* filename);


void create_serverU_socket() {
    sockfd_serverU_UDP = socket(AF_INET, SOCK_DGRAM, 0); // Create a UDP socket
    if (sockfd_serverU_UDP == FAIL) {
        perror("[ERROR] server U: can not open socket");
        exit(1);
    }
}


void init_Main_connection() {

    // Server U side information
    // Initialize server S IP address, port number
    memset(&serverU_addr, 0, sizeof(serverU_addr)); //  make sure the struct is empty
    serverU_addr.sin_family = AF_INET; // Use IPv4 address family
    serverU_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST); // Host IP address
    serverU_addr.sin_port = htons(ServerS_UDP_PORT); // Server S port number

    memset(&main_addr, 0, sizeof(main_addr)); // 确保结构体的其余部分被清零
    main_addr.sin_family = AF_INET;
    main_addr.sin_port = htons(DEST_PORT); // 替换DEST_PORT为目标端口号
    main_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST); // LOCAL_HOST已经定义为"127.0.0.1"

    
}

/**
 * Bind socket with specified IP address and port number
 */
void bind_socket() {
    if (bind(sockfd_serverU_UDP, (struct sockaddr *) &serverU_addr, sizeof(serverU_addr)) == FAIL) {
        perror("[ERROR] Server U: fail to bind Server S UDP socket");
        exit(1);
    }

    printf("The Server U is up and running using UDP on port <%d>. \n", ServerS_UDP_PORT);
}


void loadRoomData(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Unable to open the file");
        exit(EXIT_FAILURE);
    }

    while (fscanf(file, "%[^,], %d\n", roomData[roomCount].roomCode, &roomData[roomCount].available) == 2) {
        roomCount++;
        if (roomCount >= MAX_ROOMS) break; // 避免溢出
    }


    fclose(file);


}




int main() {
   
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

    for (int i = 0; i < roomCount; i++) {
        char roomStatus[100]; // one room staus
        sprintf(roomStatus, "%s, %d\n", roomData[i].roomCode, roomData[i].available);
        strcat(message, roomStatus); 
    }
    
    if (sendto(sockfd_serverU_UDP,message, sizeof(message)+1, 0, (struct sockaddr *) &main_addr,main_addr_size) < 0) {
        perror("[ERROR] Server U: fail to send write result to Main server");
        exit(1);
    }

    printf("The Server U has sent the room status to the main server.");


}
