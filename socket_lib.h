#ifndef SOCKET_LIB_H
#define SOCKET_LIB_H

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

#define MAX_SLIST_SIZE 17301224

int create_UDPSocket(int *socket_fd, int portno);
int read_UDPSocket(int *socket_fd, char* buffer, char* expected_ip, int buf_size);
int sendto_UDPSocket(int *socket_fd, int portno, char *ip, char* msg, int msg_bytes);
int close_UDPSocket(int *socket_fd);
int server_TCPSocket(int *socket_fd, int portno, char* ip);
int client_TCPSocket(int *socket_fd, int portno, char* ip);
int read_TCPSocket(int *socket_fd, char* buf, unsigned int buf_size);
int sendto_TCPSocket(int *socket_fd, char* buf, unsigned int buf_size);
int shutdown_TCPSocket(int *socket_fd);
int host_to_ip(char *hostname, char *ip);
int fd_is_valid(int fd);
char *readExactly(int fd, char *buf, int bytes_to_read, int *bytes_read);

#endif /*SOCKET_LIB_H*/