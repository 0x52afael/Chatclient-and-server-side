#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <langinfo.h>
#include <locale.h>
#include <assert.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include "socket_lib.h"
#include "PDU.h"
#include "pdu_validator.h"



#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

#define MAX_BUFF_SIZE 65535


struct clientInfo{

	unsigned short portno;
	char hostname[255];
	char ipv4[15];
	int serverFlag; //0 cs, 1 ns

}typedef clientInfo;

typedef struct session{

	char client_id[255];
	int *client_fd;
	char buff[MAX_BUFF_SIZE];
	pthread_t thread_id;

}session;


struct serverList{

	int numServers;
	serverInfo si;
	
}typedef serverList;



void errArgc(int argc);
//ss
clientInfo getServerInfo(char *argv[]); //gets serverinfo from CLi
unsigned short decToBin(unsigned short num);
unsigned short convertArrayVal(char arr[],int size);
unsigned short powerOfTen(int power);
int getServerType(char *argv[],clientInfo *si);
int setupClientConnection(int *fd, clientInfo si);
void sendGETLIST(int *client_fd);
int sendJOIN(int *client_fd,char *buff);
int readSLIST(int *client_fd, serverInfo *si);
int chooseServerConnection(serverInfo si,int *client_fd);
int connectToSpecificServer(int *fd, clientInfo si);
void whileOnline(int *client_fd);
void epochConverter();
void casePJOIN(int byte_count, char buff[],int fd);
void casePARTICIPANTS(int byte_count, char buff[],int fd);
void caseMESS(int byte_count, char buff[], int fd);
void casePLEAVE(int byte_count, char buff[], int fd);
void caseQUIT(int byte_count, char buff[], int fd);
void sendQUIT(session s);
void sendMESS(session *s, char *buff);
void *clientWriteEnd(void *sesh);
void rmNewLine(char scannedString[], char fixedString[], int size);
void filterMSG(char msg[], char filtered[],int msglen);
