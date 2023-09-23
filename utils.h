#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#ifndef __UTILS_H__
#define __UTILS_H__
void print_ipv4_address(struct sockaddr_in);
void trim_ln(char*, int);
void overwrite_stdout();

#endif