#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <signal.h>
#include "types.h"
#include "utils.h"

#define MAX_CLIENTS 200

static int uid = 10;  // user id. Unique for every user
static _Atomic unsigned int client_count = 0;  // ensure atomicity

typedef struct {
    pthread_mutex_t lock;  // lock for client table
    client_t* head;  // linked list of clients, points to the head
} client_table_t;

client_table_t* client_table;  // store client information

// initialize client table
void init_client_table() {
    client_table = (client_table_t*)malloc(sizeof(client_table_t));
    pthread_mutex_init(&client_table->lock, NULL);
    client_table->head = NULL;  // head initially points to NULL

    return;
}

// check if the chat room is full
int room_is_full() {
    return (client_count >= MAX_CLIENTS);
}

// add client to list
// singly linked list
void add_to_list(client_t* client) {
    pthread_mutex_lock(&client_table->lock);   // lock the table
    if (client_table->head == NULL) {
        client_table->head = client;
    } else {
        client->next = (struct client_t*)client_table->head;
        client_table->head = client;
    }
    pthread_mutex_unlock(&client_table->lock); // unlock the table
}

// remove client from list
void remove_from_list(int uid) {
    pthread_mutex_lock(&client_table->lock);
    client_t* cursor_prev = NULL;  // one node before cursor
    client_t* cursor = client_table->head;

    while (cursor != NULL) {
        if (cursor->uid == uid) {
            if (cursor_prev == NULL) {  // only one left in the list
                client_table->head = NULL;  // head is NULL now
            } else {
                cursor_prev->next = cursor->next;
            }
            break;
        }
        cursor = (client_t*)cursor->next;  // move cursor to next
    }
    pthread_mutex_unlock(&client_table->lock);
}

// send message to everyone
void send_message(char* message, int uid) {
    pthread_mutex_lock(&client_table->lock);
    client_t* cursor = client_table->head;
    while (cursor != NULL) {
        if (cursor->uid != uid) {
            if (write(cursor->sockfd, message, strlen(message)) < 0) {
                perror("write");
                break;
            }
        }
        cursor = (client_t*)cursor->next;  // move cursor to next
    }
    pthread_mutex_unlock(&client_table->lock);
}

// handle client
void* handle_client(void* arg) {
    char buffer[BUFFER_SIZE];  // message buffer
    char name[NAME_LEN];       // name of the user
    int leave = 0;  // flag to determine whether the user left or not
    
    client_count++;  // increment client count
    client_t* client = (client_t*)arg;

    // improper name, the user leaves
    if (recv(client->sockfd, name, NAME_LEN, 0) <= 0 || strlen(name) < 1 || strlen(name) >= NAME_LEN - 1) {
        perror("Name must not be blank nor longer than 32\n");
        leave = 1;
    } else {  // proper name, join the room
        strcpy(client->name, name);
        sprintf(buffer, "%s joined the chat!\n", client->name);
        printf("%s", buffer);
        send_message(buffer, client->uid);  // send the join message to everyone except the joined user
    }
    bzero(buffer, BUFFER_SIZE);

    while(1) {
        if (leave) break;

        int received = recv(client->sockfd, buffer, BUFFER_SIZE, 0);  // receive message from the user

        if (received > 0) {
            if (strlen(buffer) > 0) {  // a message has been sent from user
                send_message(buffer, client->uid);  // send to everyone in the room
                trim_ln(buffer, strlen(buffer));  // trim the string
                printf("%s\n", buffer);  // print it out
            }
        } else if (received == 0 || strcmp(buffer, "exit") == 0) {  // the user wants to exit
            sprintf(buffer, "%s has left\n", client->name);
            printf("%s", buffer);
            send_message(buffer, client->uid);
            leave = 1;
        } else {  // something went wrong
            perror("recv");
            leave = 1;
        }

        bzero(buffer, BUFFER_SIZE);  // clean the buffer
    }

    close(client->sockfd);  // close TCP connection
    remove_from_list(client->uid);  // remove client from list
    free(client);  // free client resources
    client_count--;
    pthread_detach(pthread_self());

    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 2) {  // must specify port number to start the server
        printf("Usage: %s <port>\n", argv[0]);  // show you need to add port number as an argument
        return EXIT_FAILURE;
    }

    char* ip = "127.0.0.1";  // localhost IP address
    int port = atoi(argv[1]);  // port number

    int option = 1;  // store socket options
    int listenfd;  // TCP listen fd
    int connectionfd;  // TCP connection fd

    struct sockaddr_in server_addr;  // address of the server (AF_INET is used)
    struct sockaddr_in client_addr;  // address of the client (AF_INET is used)
    pthread_t tid;  // thread ID

    // set the socket to listen to other clients
    listenfd = socket(AF_INET, SOCK_STREAM, 0);  // TCP protocol using IPV4
    server_addr.sin_family = AF_INET;  // fixed when using AF_INET
    server_addr.sin_addr.s_addr = inet_addr(ip);  // inet_addr (convert ipv4 looking string to a in_addr type)
    server_addr.sin_port = htons(port);  // htons (convert port number to network byte sequence)

    // use the same address when rebooting after an unexpected shut down
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char*)&option, sizeof(option)) < 0) {
        perror("setsockopt");
        return EXIT_FAILURE;
    }

    // if bind listenfd to server address
    if (bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        return EXIT_FAILURE;
    }

    // listen up to 10 connection at the same time
    if (listen(listenfd, 10) < 0) {
        perror("listen");
        return EXIT_FAILURE;
    }

    printf("=== CHAT ===\n");

    init_client_table();  // initialize client table

    while (1) {
        socklen_t client_len = sizeof(client_addr);
        connectionfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_len);  // accept connection from client

        if (room_is_full()) {  // if the room is full
            printf("Server is full. Connection Rejected\n");
            close(connectionfd);
            continue;
        }

        // get client information
        client_t* client = (client_t*)malloc(sizeof(client_t));
        client->address = client_addr;
        client->sockfd = connectionfd;
        client->uid = uid++;  // unique uid
        client->next = NULL;  // initialize next to null at first

        add_to_list(client);
        pthread_create(&tid, NULL, &handle_client, (void*)client);  // handle client in a different thraed

        sleep(1);  // tiny pause
    }

    return EXIT_SUCCESS;
}

