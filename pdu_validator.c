#include "pdu_validator.h"


/*
* Returns the nr of pads required to fill an entire word (4 bytes)
*/
int getNumberPads(int msg_len)
{
	int rest = (msg_len % 4);
	return (rest == 0) ? 0 : (4 - rest); 
}

/*
* Calculates checksum. 
*/
unsigned char calculateChecksum(unsigned char *pdu, int size){

    unsigned short checkSum = 0;
    for(int i = 0; i < size; i++){
        checkSum += (unsigned char)pdu[i];
    }

	checkSum = checkSum % 255;
	
   	checkSum = ~checkSum;
   	checkSum = checkSum & 0xFF;

    return (unsigned char)checkSum;

}

/*Checks that msg[0] to msg[nr_pads-1] are PADS*/
int checkPads(char *msg, int nr_pads)
{
	for(int i = 0; i<nr_pads; i++)
	{
		if(msg[i] != PAD)
			return 0;
	}
	return 1;
}

int validate_ACK_NOTREG(int nr_bytes, char* pduMsg)
{
	if(nr_bytes != 4)
		return 0;

	return checkPads(&pduMsg[1], 1);
}

int validate_SLIST(int nr_bytes, char* pduMsg)
{
	if(nr_bytes < 4)
		return 0;

	if(!checkPads(&pduMsg[1], 1))
		return 0;

	unsigned short nr_servers;
	memcpy(&nr_servers, &pduMsg[2], sizeof(unsigned short));
	nr_servers = ntohs(nr_servers);

	int index, name_len, nr_pads;
	index = 4;
	for(int i = 0; i < (int)nr_servers; i++)
	{
		
		if(nr_bytes < (index + 8))
			return 0;
		name_len = (int) pduMsg[index + 7];
		nr_pads = getNumberPads(name_len);
		
		if(nr_bytes < (index + 8 + name_len + nr_pads))
			return 0;

		if(!checkPads(&pduMsg[index + 8 + name_len], nr_pads))
			return 0;

		index = (index + 8 + name_len + nr_pads);
	}

	return 1;
}

int validate_JOIN(int nr_bytes, char* pduMsg)
{
	if(nr_bytes < 4)
		return 0;

	return checkPads(&pduMsg[2], 2);
}

int validate_MESS(int nr_bytes, char* pduMsg)
{
	if(nr_bytes < 16)
	{
		printf("INVALID MESS: 1 \n");
		return 0;
	}

	unsigned short msg_len;
	int id_len, id_pads, msg_pads;
	id_len = (int)pduMsg[2];

	memcpy(&msg_len, &pduMsg[4], sizeof(unsigned short));
	msg_len = ntohs(msg_len);

	if(!checkPads(&pduMsg[1], 1) || !checkPads(&pduMsg[6], 2))
	{
		printf("INVALID MESS: 2\n");
		return 0;
	}

	msg_pads = getNumberPads((int)msg_len);
	id_pads = getNumberPads(id_len);
	

	if(nr_bytes != (12 + msg_len + msg_pads + id_len + id_pads))
	{
		printf("nr_bytes = %d\n",nr_bytes);
		printf("should be: %d\n",(12 + msg_len + msg_pads + id_len + id_pads));
		printf("INVALID MESS: 3\n");
		return 0;
	}

	if(!checkPads(&pduMsg[12 + msg_len], msg_pads))
	{
		printf("INVALID MESS: 4\n");
		return 0;
	}
	
	unsigned short csum = 0;

	if(!( csum = calculateChecksum((unsigned char *)pduMsg, nr_bytes) == 255))
	{	
		
		printf("INVALID MESS: CHECKSUM WAS %d\n",csum);
		
		return 0;
	}

	return checkPads(&pduMsg[12 + msg_len + msg_pads + id_len], id_pads);
}

int validate_QUIT(int nr_bytes, char* pduMsg)
{
	if(nr_bytes != 4)
		return 0;

	return checkPads(&pduMsg[1], 3);
}

int validate_PJOIN_PLEAVE(int nr_bytes, char* pduMsg)
{
	if(nr_bytes < 2) 
		return 0;

	int id_len = (int)pduMsg[1];
	int nr_pads = getNumberPads(id_len);

	/*8 constant bytes + client id length with pads*/
	if(nr_bytes != (8 + id_len + nr_pads))
		return 0;

	if(!checkPads(&pduMsg[2], 2))
		return 0;

	return checkPads(&pduMsg[8 + id_len], nr_pads);
}
/*
Kan inte ett fel vara att det är dubbla Null tecken efter användarnamn?
*/
int validate_PARTICIPANTS(int nr_bytes, char* pduMsg)
{
	unsigned int part_count = 0;

	if(nr_bytes < 4)
		return 0;


	unsigned short id_len;
	
	memcpy(&id_len, &pduMsg[2], sizeof(unsigned short));
	id_len = ntohs(id_len);
	
	unsigned int nr_participants = (unsigned char)pduMsg[1];
	int nr_pads = getNumberPads((int)id_len);

	if(nr_bytes != (4 + id_len + nr_pads))
		return 0;


	if(nr_bytes == 4 && nr_participants == 0)
		return 1;

	if(pduMsg[4] == PAD && nr_participants > 0)
		return false;


	for(int i = 0; i < id_len; i++)
	{
		if(pduMsg[i+4] == PAD)
		{
			part_count++;
			/*If theres another pad right after it must be the last participant*/
			if(pduMsg[i+5] == PAD && (i+1) != id_len)
			{
				printf("val_PARTICIPANTS 3 was incorrect\n");
				return false;
			}
		}
	}

	if(part_count != nr_participants)
	{
		printf("part_count was: %d, nr_participants was :%d\n", part_count, nr_participants);
		printf("val_PARTICIPANTS 4 was incorrect\n");
		return false;
	}

	return checkPads(&pduMsg[4 + id_len], nr_pads);
}


int validate_pdu(int OP_NR, int nr_bytes, char *pduMsg)
{
	switch(OP_NR)
	{
		case ACK:
			return validate_ACK_NOTREG(nr_bytes, pduMsg);
		case NOTREG:
			return validate_ACK_NOTREG(nr_bytes, pduMsg);
		case SLIST:
			return validate_SLIST(nr_bytes, pduMsg);
		case MESS:
			return validate_MESS(nr_bytes, pduMsg);
		case QUIT:
			return validate_QUIT(nr_bytes, pduMsg);
		case JOIN:
			return validate_JOIN(nr_bytes, pduMsg);
		case PJOIN:
			return validate_PJOIN_PLEAVE(nr_bytes, pduMsg);
		case PLEAVE:
			return validate_PJOIN_PLEAVE(nr_bytes, pduMsg);
		case PARTICIPANTS:
			return validate_PARTICIPANTS(nr_bytes, pduMsg);
		default:
			return 0;
	}
}
