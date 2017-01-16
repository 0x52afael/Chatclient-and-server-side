#define _BSD_SOURCE

#ifndef CHAT_SERVER_H
#define CHAT_SERVER_H

#include "socket_lib.h"
#include "msgqueue.h"
#include "PDU.h"
#include "pdu_validator.h"
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <pthread.h>

msg_Queue messageQueue;
client clientList[255];
pthread_mutex_t clientList_LOCK;
pthread_mutex_t messageQueue_LOCK;

typedef struct connectionInfo {
	char hostName[256];
	int nsPort;
	char myName[256];
	int myPort;

} connectionInfo;


typedef struct msgQueueItem {

	char* msg;
	unsigned int byte_size;

} QueueItem;

int register_Server(int* fd, int* idNum, connectionInfo *ci);
int server_Alive(int* fd, char nrClients, unsigned short idNum);
void handleMessage(int nr_bytes, char *buf, client *c);
char *handleByteStream(char *pdu, client *c, int *bytes_read);

#endif /*CHAT_SERVER_H*/