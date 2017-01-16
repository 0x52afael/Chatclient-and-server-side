#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h> //htons
#include <assert.h>
#include <stdbool.h>

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


int validate_pdu(int OP_NR, int nr_bytes, char *pduMsg);
int validate_SLIST(int nr_bytes, char* pduMsg);
int validate_ACK_NOTREG(int nr_bytes, char* pduMsg);
int validate_JOIN(int nr_bytes, char* pduMsg);
int validate_MESS(int nr_bytes, char* pduMsg);
int validate_QUIT(int nr_bytes, char* pduMsg);
int validate_PJOIN_PLEAVE(int nr_bytes, char* pduMsg);
int validate_PARTICIPANTS(int nr_bytes, char* pduMsg);