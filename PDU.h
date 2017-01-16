#ifndef PDU_H
#define PDU_H

#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <langinfo.h>
#include <locale.h>
#include <assert.h>
#include <time.h>


//OP-code macros
#define REG 0
#define ACK 1
#define ALIVE 2
#define NOTREG 100
#define GETLIST 3
#define SLIST 4
#define MESS 10
#define QUIT 11
#define JOIN 12
#define PJOIN 16
#define PLEAVE 17
#define PARTICIPANTS 19
#define PAD 0x00


/*
Ett identitetsnamn kan inte vara längre än 255 tecken (bytes).
• Ett servernamn kan inte vara längre än 255 tecken (bytes).
• En namnserver kan bara hålla koll på 65535 servrar.
• En server kan bara ha 255 anslutna klienter i sessionen.
• Ett meddelande kan inte vara längre än 65535 tecken (bytes)
*/


struct pdu_REG{

	char op_code;
	unsigned short portnum;
	char servNL; //to become a bit representative of nr of chars of serv name
	char servName[255];

}typedef pdu_REG;



struct pdu_ALIVE{

	char op_code;
	char nrOfClients;
	unsigned short identityNum;

}typedef pdu_ALIVE;

struct pdu_ACK{

	char op_code;
	unsigned short identityNum;

}typedef pdu_ACK;

struct pdu_NOTREG{

	char op_code;
	unsigned short identityNum;

}typedef pdu_NOTREG;

struct pdu_GETLIST{

	char op_code;

}typedef pdu_GETLIST;

struct pdu_JOIN{

	char op_code;
	char identLen;
	char *identity;

}typedef pdu_JOIN;

struct pdu_PJOIN {

	char op_code;
	char identLen;
	unsigned long timestamp;
	char *identity;
} typedef pdu_PJOIN;

struct client {

	char name[255];
	unsigned int idNum;
	int totalRead;
	int fd;
	int hasJoined;

}typedef client;

struct pdu_PARTICIPANTS{

	char op_code;
	char numParticipants;
	unsigned short id_len;
	client participants[255];
	

}typedef pdu_PARTICIPANTS;

struct pdu_MESS{

	char op_code;
	char idLen;
	unsigned char crc;
	unsigned short msglen;
	unsigned long timestamp; //kan behöva vara en annan datatyp.
	char *msg;
	char client_id[255];


}typedef pdu_MESS;

struct pdu_PLEAVE {

	char op_code;
	char id_len;
	unsigned long timestamp;
	char *identity;
} typedef pdu_PLEAVE;

struct server{

	char ipv4[15];
	unsigned short portNum;
	char nrClients;
	char servNL; //server name lenght
	char servername[255];

}typedef server;

struct serverInfo{

    char op_code;
	unsigned short nrOfServers; 
	server *serv;

}typedef serverInfo;


char* create_REG(pdu_REG r, int* byte_size);
char* create_ACK(pdu_ACK a, int* byte_size);
char* create_ALIVE(pdu_ALIVE al, int* byte_size);
char* create_GETLIST(int* byte_size);
char* create_MESS(pdu_MESS msg, int choice, int* byte_size);
char* create_QUIT(int* byte_size);
char* create_JOIN(pdu_JOIN j, int* byte_size);
char* create_PJOIN(pdu_PJOIN pj, int* byte_size);
char* create_PLEAVE(pdu_PJOIN pj, int* byte_size);
char* create_PARTICIPANTS(pdu_PARTICIPANTS p, client clients[], int* byte_size);


serverInfo parse_SLIST();
pdu_MESS parse_MSG(char buffer[], int fromServer);
pdu_REG *parse_REG(char *reg_msg); //bara för att testa reg? behövs inte
pdu_ACK *parse_ACK(char *ack_msg);
int parse_QUIT(char buff[]);
pdu_NOTREG parse_NOTREG(char buffer[]);
pdu_JOIN *parse_JOIN(char *join_msg);
pdu_PJOIN *parse_PJOIN(char *pjoin_msg);
pdu_PLEAVE *parse_PLEAVE(char *msg);
pdu_PARTICIPANTS *parse_PARTICIPANTS(char *msg);


//BOB  bits of buffer
void readBOB(char buffer[],char buffmem[],int start, int end); 
char readCharBytes(char buffer[],int index);
int calculateMsgPad(int msgEndIndex);
void clearBuffPlaceholder(char tmp[], int end);
int parse_QUIT(char buff[]);
int getNrPads(int msg_len);
int convert_ip(unsigned char n[4], char* ip);
unsigned char checkSumCalculator(unsigned char *pdu, int size);

#endif /*PDU_H*/








































