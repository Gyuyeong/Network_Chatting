#ifndef __TYPES_H__
#define __TYPES_H__

#define BUFFER_SIZE 2048
#define NAME_LEN 32

typedef struct {
    struct sockaddr_in address;  // socket address of the client
    int sockfd;  // connection socket descriptor for this client
    int uid;  // uid of this client
    char name[NAME_LEN];  // name of the client
    struct client_t* next;  // next pointer
} client_t;

typedef struct {
    int room_id;  // room id, unique
    client_t* head;  // linked list of clients, points to the head
    int client_count;  // number of clients in the room
} room_t;

#endif