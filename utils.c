#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>

// print IPV4 address
void print_ipv4_address(struct sockaddr_in addr) {
    printf("%d.%d.%d.%d", 
            addr.sin_addr.s_addr & 0xff,
            (addr.sin_addr.s_addr & 0xff00) >> 8,
            (addr.sin_addr.s_addr & 0xff0000) >> 16,
            (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

// trim newline at the end and put null character
void trim_ln(char* arr, int len) {
    for (int i=0; i<len; i++) {
        if (arr[i] == '\n') {
            arr[i] = '\0';
            break;
        }
    }

    return;
}

void overwrite_stdout() {
    printf("\r%s", ">>> ");
    fflush(stdout);
}