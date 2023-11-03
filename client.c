#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include "utils.h"
#include "types.h"

volatile sig_atomic_t flag = 0;  // flag to indicate whether aborted or not
int sockfd = 0;  // socket connection with the server
char name[NAME_LEN];  // name of the client

// when ctrl_c is inserted, change the flag to exit
void catch_ctrl_c_and_exit() {
    flag = 1;
}

// handle message sent to server
void send_message_handler() {
    char buffer[BUFFER_SIZE] = {};
    char message[BUFFER_SIZE + NAME_LEN + 4] = {};  // message to send

    while (1) {
        overwrite_stdout();
        fgets(buffer, BUFFER_SIZE, stdin);
        trim_ln(buffer, BUFFER_SIZE);

        if (strcmp(buffer, "exit") == 0) break;
        else {
            sprintf(message, "%s: %s\n", name, buffer);
            send(sockfd, message, strlen(message), 0);  // send message
        }

        bzero(buffer, BUFFER_SIZE);  // clear buffer
        bzero(message, BUFFER_SIZE + NAME_LEN + 4);  // clear message
    }
    catch_ctrl_c_and_exit(2);  // catch ctrl c
}


// handle received messages
void receive_message_handler() {
    char message[BUFFER_SIZE] = {};
    while (1) {
        int received = recv(sockfd, message, BUFFER_SIZE, 0);  // receive message from server

        if (received > 0) {  // received something
            printf("%s ", message);
            overwrite_stdout();
        } else if (received == 0) {  // did not receive anything
            break;
        }
        bzero(message, BUFFER_SIZE);  // clear buffer
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s <IP Address> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char* ip = argv[1];   // get ip address
    int port = atoi(argv[2]); // get port number

    signal(SIGINT, catch_ctrl_c_and_exit);  // if SIGINT, go to that function instead

    printf("Enter your name: ");
    fgets(name, NAME_LEN, stdin);

    trim_ln(name, strlen(name));

    if (strlen(name) > NAME_LEN - 1 || strlen(name) < 1) {
        perror("Name must not be empty nor longer than 32 characters");
        return EXIT_FAILURE;
    }

    // initialize server address
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);  // IPv4, TCP
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    // attempt to connect to the server
    int connected = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (connected == -1) {
        perror("connect");
        return EXIT_FAILURE;
    }

    send(sockfd, name, NAME_LEN, 0);  // send the name

    printf("=== CHAT ===\n");

    pthread_t send_message_thread;
    if (pthread_create(&send_message_thread, NULL, (void*)send_message_handler, NULL) != 0) {
        perror("send message");
        return EXIT_FAILURE;
    }

    pthread_t receive_message_thread;
    if (pthread_create(&receive_message_thread, NULL, (void*)receive_message_handler, NULL) != 0) {
        perror("receive message");
        return EXIT_FAILURE;
    }

    while (1) {
        if (flag) {
            printf("Bye\n");
            break;
        }
    }

    close(sockfd);

    return EXIT_SUCCESS;
}
