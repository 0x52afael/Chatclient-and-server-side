#include "PDU.h"


void print_bits(unsigned int x, unsigned short size)
{
    int i;
    for(i=8*size-1; i>=0; i--) {
        (x & (1 << i)) ? putchar('1') : putchar('0');
    }
    printf("\n");
}

/*
* Returns the nr of pads required to fill an entire word (4 bytes)
*/
int getNrPads(int msg_len)
{
	int rest = (msg_len % 4);
	return (rest == 0) ? 0 : (4 - rest); 
}

/*
* Creates a reg that is sent to the nameserver to register.
*/
char* create_REG(pdu_REG r, int* byte_size)
{
	int nr_pads = getNrPads( (int)r.servNL );
	r.portnum = htons(r.portnum);
	int mem_size = 4 + (int)r.servNL + nr_pads;

	char* pdu = malloc(mem_size);

	pdu[0] = r.op_code;
	pdu[1] = r.servNL;

	/*Store port_nr in pdu*/
	memcpy(&pdu[2], &r.portnum, 2);

	for(int i = 4; i < 4 + (int)r.servNL; i++)
	{
		pdu[i] = r.servName[i-4];
	}

	for(int i = 4 + (int)r.servNL; i < mem_size; i++)
	{
		pdu[i] = PAD;
	}

	*byte_size = mem_size;

	return pdu;
}

/*
* Just used for testing
*/
pdu_REG *parse_REG(char *reg_msg) //bara för att testa reg? behövs inte
{
	unsigned short port_nr;
	int name_len;
	pdu_REG *pdu = malloc(sizeof(pdu_REG));
	
	pdu->op_code = reg_msg[0];
	pdu->servNL = reg_msg[1];

	name_len = (int) reg_msg[1];

	memcpy(&port_nr, &reg_msg[2], sizeof(unsigned short));
	pdu->portnum = ntohs(port_nr);

	for(int i = 0; i< name_len; i++)
	{
		pdu->servName[i] = reg_msg[4+i];
	}

	return pdu;
}

/*Creates ACK PDU*/
char* create_ACK(pdu_ACK a, int* byte_size)
{
	char* pdu = malloc(4);
	a.identityNum = htons(a.identityNum);

	pdu[0] = a.op_code;
	pdu[1] = PAD;
	memcpy(&pdu[2], &a.identityNum, 2);

	*byte_size = 4;

	return pdu;
}

/*Parses an ACK to the pdu_ACK struct*/
pdu_ACK *parse_ACK(char *ack_msg)
{
	pdu_ACK *pdu = malloc(sizeof(pdu_ACK));
	unsigned short id_num = 0;

	pdu->op_code = ack_msg[0];
	
	memcpy(&id_num, &ack_msg[2], sizeof(unsigned short));
	pdu->identityNum = ntohs(id_num);

	return pdu;
}

/*Creates an ALIVE pdu*/
char* create_ALIVE(pdu_ALIVE al, int* byte_size)
{
	char* pdu = malloc(4);
	al.identityNum = htons(al.identityNum);

	pdu[0] = al.op_code;
	pdu[1] = al.nrOfClients;
	memcpy(&pdu[2], &al.identityNum, 2);

	*byte_size = 4;

	return pdu;
}

/*Creates a GETLIST pdu.*/
char* create_GETLIST(int* byte_size)
{
	char* pdu = malloc(4);

	pdu[0] = GETLIST;
	pdu[1] = PAD;
	pdu[2] = PAD;
	pdu[3] = PAD;

	*byte_size = 4;
	
	return pdu;
}

/* Choice 1 = server, 0 = client.
* Creates a char string containing the information required in the MESS pdu. 
*/
char* create_MESS(pdu_MESS m, int choice, int* byte_size)
{
	int msg_pads = getNrPads((int)m.msglen);
	int id_pads = getNrPads((unsigned char)m.idLen);
	int mem_size = (int)m.msglen + msg_pads + 12;

	unsigned short msg_fixed = (htons(m.msglen));
	unsigned long timestamp_fixed = htonl(m.timestamp);

	if(choice == 1) 
	{
		mem_size = (mem_size + id_pads + (int)m.idLen); 
	}

	*byte_size = mem_size;

	char *pdu = malloc(mem_size);

	pdu[0] = MESS;
	pdu[1] = PAD;
	pdu[2] = (unsigned char)m.idLen;
	pdu[3] = PAD;	
	memcpy(&pdu[4], &msg_fixed, 2);
	pdu[6] = PAD;
	pdu[7] = PAD;
	
	if(choice == 0){
		timestamp_fixed = 0;
	}
	
	memcpy(&pdu[8], &timestamp_fixed, 4);

	int index = 12;

	for(int i = index; i < ( (int)m.msglen + index); i++)
	{
		pdu[i] = m.msg[i-index];
       
	}

	index = (int)m.msglen + index; 
	
	for(int i = index; i < (index + msg_pads); i++)
	{
		pdu[i] = PAD;
	}

	/*Only fill the client id if it's the server creating this message.*/
	if(choice == 1)
	{
		index = index + msg_pads;
		for(int i = index; i < (index + (int)m.idLen); i++)
		{
			pdu[i] = (unsigned char)m.client_id[i - index];
		}

		index = index + (int)m.idLen;

		for(int i = index; i< (index + id_pads); i++)
		{
			pdu[i] = PAD;
		}
	}

	
	pdu[3] = checkSumCalculator((unsigned char*)pdu, *byte_size);

	return pdu;
}


/*
Creates a text string for the pdu QUIT
*/
char* create_QUIT(int* byte_size)
{
	char* pdu = malloc(4);

	pdu[0] = QUIT;
	pdu[1] = PAD;
	pdu[2] = PAD;
	pdu[3] = PAD;

	*byte_size = 4;

	return pdu;
}
/*
Will mostly be used to see if the op code of a in comming message is QUIT
or not.
*/
int parse_QUIT(char buff[]){

	if( (buff[0]) == 11){
		return 1;
	}

	return 0;

}

pdu_NOTREG parse_NOTREG(char buffer[]){

	unsigned short tmp;
	pdu_NOTREG nr;
	nr.op_code = buffer[0];
	memcpy(&tmp, &buffer[2], sizeof(unsigned short));
	nr.identityNum = ntohs(tmp);

	return nr;
}

/*
Creates a JOIN pdu message string.
The arguments are lenght of idenity and the identity.
*/
char* create_JOIN(pdu_JOIN j, int* byte_size)
{
	int nr_pads =  getNrPads((int)j.identLen);
	int mem_size = 4 + (int)j.identLen + nr_pads;

	*byte_size = mem_size;

	char* pdu = malloc(mem_size);

	pdu[0] = j.op_code;
	pdu[1] = j.identLen;
	pdu[2] = PAD;
	pdu[3] = PAD;

	for(int i = 4; i < 4 + (int)j.identLen; i++)
	{
		pdu[i] = j.identity[i-4];
	}

	for(int i = 4 + (int)j.identLen; i < mem_size; i++)
	{
		pdu[i] = PAD;
	}

	return pdu;
}
/*
Parses JOIN text message and returns the struct containing the information
from the JOIN pdu message.
*/
pdu_JOIN *parse_JOIN(char *join_msg)
{
	pdu_JOIN *pdu = malloc(sizeof(pdu_JOIN));
	int id_len = (int)join_msg[1];

	pdu->op_code = join_msg[0];
	pdu->identLen = join_msg[1];
	pdu->identity = malloc(id_len + 1);

	for(int i = 0; i<id_len; i++)
	{
		pdu->identity[i] = join_msg[i+4];
	}

	pdu->identity[id_len] = '\0';

	return pdu;
}
/*
Creates the PJOIN text string to be used later when sending PJOIN.
Arguments to the function are timestamp, client id and lenght of id.
*/
char* create_PJOIN(pdu_PJOIN pj, int* byte_size)
{
	int nr_pads = getNrPads((int) pj.identLen);
	int mem_size = 8 + nr_pads + (int)pj.identLen; //kan bli fel här?
	pj.timestamp = htonl(pj.timestamp);

	*byte_size = mem_size;
	char* pdu = malloc(mem_size);

	pdu[0] = PJOIN;
	pdu[1] = pj.identLen;
	pdu[2] = PAD;
	pdu[3] = PAD;

	memcpy(&pdu[4], &pj.timestamp, 4);

	for(int i = 8; i < 8 + (int)pj.identLen; i++)
	{
		pdu[i] = pj.identity[i-8];
	}

	for(int i = 8 + (int)pj.identLen; i< mem_size; i++)
	{
		pdu[i] = PAD;
	}

	return pdu;
}
/*
Parses a PJOIN text string message and creates a PJOIN struct from the string
information. Then returns the pdu struct to caller.
*/
pdu_PJOIN *parse_PJOIN(char *pjoin_msg)
{
	pdu_PJOIN *pdu = malloc(sizeof(pdu_PJOIN));

	int id_len = (int)pjoin_msg[1];
	pdu->identity = malloc(id_len + 1);

	pdu->op_code = pjoin_msg[0];
	pdu->identLen = pjoin_msg[1];

	memcpy(&pdu->timestamp, &pjoin_msg[4], sizeof(unsigned long));
	pdu->timestamp = ntohl(pdu->timestamp);

	for(int i = 0; i<id_len; i++)
	{
		pdu->identity[i] = pjoin_msg[i+8];
	}

	pdu->identity[id_len] = '\0';
	return pdu;
}
/*
Takes argument for creating a pdu PLEAVE and creates a text string
which is returned for use.
This function uses the create_PJOIN pdu function to create PLEAVE because of
their resemblence in setup.
*/
char* create_PLEAVE(pdu_PJOIN pj, int* byte_size)
{
	char *pdu = create_PJOIN(pj, byte_size);
	pdu[0] = PLEAVE;

	return pdu;
}
/*
parses the text string msg for PLEAVE pdu and saves into the pdu struct
and returns the struct.
*/
pdu_PLEAVE *parse_PLEAVE(char *msg)
{
	int id_len = (int)msg[1];
	pdu_PLEAVE *pdu = malloc(sizeof(pdu_PLEAVE));
	pdu->identity = malloc(id_len +1);

	pdu->op_code = msg[0];
	pdu->id_len = msg[1];
	memcpy(&pdu->timestamp, &msg[4], sizeof(unsigned long));
	pdu->timestamp = ntohl(pdu->timestamp);

	for(int i = 0; i<id_len; i++)
	{
		pdu->identity[i] = msg[i+8];
	}

	pdu->identity[id_len] = '\0';
	return pdu;
}

/*
* Creates a char string from the information about the participants.
* Length is length of all names including null terminator, ie sum of strlen + nr participants.
*/
char* create_PARTICIPANTS(pdu_PARTICIPANTS p, client clients[], int* byte_size)
{
	unsigned short len_fixed = htons(p.id_len);

	int pads = getNrPads((unsigned int)p.id_len);
	int index = 0;
	//4 (header) + length of names + one null per client + pads
	int mem_size = (4 + (unsigned int)p.id_len  + pads);

	*byte_size = mem_size;
	char* pdu = malloc(mem_size);

	pdu[0] = p.op_code;
	pdu[1] = (unsigned char)p.numParticipants;
	memcpy(&pdu[2], &len_fixed, 2);
	index = 4;

	for(int i = 0; i<255; i++)
	{
		if(clients[i].idNum == 0)
			continue;
		
		for(int j = 0; j<strlen(clients[i].name); j++)
		{
			pdu[index + j] = clients[i].name[j];
		}
		pdu[index + strlen(clients[i].name)] = PAD;
		index = (index + strlen(clients[i].name) + 1);
	}

	for(int i = index; i < index + pads; i++)
	{
		pdu[i] = PAD;
	}

	return pdu;
}

/*
* Parses a char string containing information about the participants, 
* and saves the information in the struct pdu_PARTICIPANTS. 
*/
pdu_PARTICIPANTS *parse_PARTICIPANTS(char *msg)
{
	pdu_PARTICIPANTS *pdu = malloc(sizeof(pdu_PARTICIPANTS));
	int nr_clients = (unsigned char)msg[1];

	pdu->op_code = msg[0];
	pdu->numParticipants = (unsigned char)msg[1];

	memcpy(&pdu->id_len, &msg[2], sizeof(unsigned short));
	pdu->id_len = ntohs(pdu->id_len);

	int index = 4;

	for(int i = 0; i< nr_clients; i++)
	{
		char tmp[255];
		int tmpIndex = 0;

		while(msg[index] != PAD)
		{
			tmp[tmpIndex] = msg[index];

			tmpIndex++;
			index++;
		}
		tmp[tmpIndex] = PAD;
		strcpy(pdu->participants[i].name, tmp);

		index++;
	}

	return pdu;
}



/*
Parse a long text char array (buffer) and divides each separate part
into a serverInfo struct which contains nr of servers and their respective
server information such as address, port number, clients connected and
server name.
*/
serverInfo parse_SLIST(char buffer[]){

	serverInfo si;
	int bufferOffset = 4;
	char temp[255];
	si.op_code = readCharBytes(buffer,0);
	readBOB(buffer,temp,2,4);
	si.nrOfServers = (buffer[2] << 8) | buffer[3];

	memcpy(&si.nrOfServers, temp, sizeof(unsigned short));
	si.nrOfServers = ntohs(si.nrOfServers);


	si.serv = calloc( si.nrOfServers, sizeof(server) );
	// this loop reads every addr,port,nr of clients, server name lenght and
	//server name into a server struct
	
	for(int i = 0; i < si.nrOfServers; i++){ 
		
		//read addr and save into struct
		readBOB(buffer,temp,bufferOffset, (bufferOffset+4) ); //read addr
		unsigned long tmp2;

		memcpy(&tmp2, &temp, sizeof(unsigned long));
		//tmp2 = ntohl(tmp2);
		unsigned char addr_placeholder[4];

		memcpy(&addr_placeholder, &tmp2, 4);

		convert_ip(addr_placeholder, si.serv[i].ipv4);

		clearBuffPlaceholder(temp,4); 
		unsigned short portNum = 0;
		bufferOffset += 4; //offset after addr

		memcpy( &portNum, &buffer[bufferOffset], sizeof(unsigned short) );

		si.serv[i].portNum = ntohs(portNum); //set it to host machine
											 //byte order
		
		bufferOffset += 2; //offset after portno
		//number of clients this server has connected
		si.serv[i].nrClients = readCharBytes(buffer, bufferOffset);
		
		bufferOffset += 1; //read server name length
		si.serv[i].servNL = readCharBytes(buffer,bufferOffset);
		
		bufferOffset += 1;

		int snPad = getNrPads((int)si.serv[i].servNL);
		int name_len = (int)si.serv[i].servNL;


		int k = 0;//read the server name

		for(int j = bufferOffset; j < (name_len+bufferOffset); j++){
			
			si.serv[i].servername[k] = buffer[j];
			k++;
		}

		bufferOffset += name_len+snPad;
		
	}
	
	return si;
}
/*
reads a long text buffer and parses it into a pdu_message struct.
takes argument of the text buffer and if the message is from server or not.
(same function will be used in both server and client side).

if its from server then it contains server id after message.

*/
pdu_MESS parse_MSG(char buffer[],int fromServer){

	pdu_MESS retInfo;

	char temp[255];
	unsigned short msglen = 0;
	unsigned int msg_len = 0;


	retInfo.op_code = readCharBytes(buffer,0);
	retInfo.idLen = readCharBytes(buffer,2);
	retInfo.crc = readCharBytes(buffer,3);

	readBOB(buffer,temp,4,6);
	memcpy(&msglen, temp, sizeof(unsigned short));
	retInfo.msglen = ntohs(msglen);
	clearBuffPlaceholder(temp,2);
	readBOB(buffer,temp,8,12);
	unsigned long ts = 0;

	memcpy(&ts,temp, sizeof(unsigned long)); //read the memory of the 4 byte
	retInfo.timestamp = ntohl(ts); //buffer and bit shifts everything into the
	clearBuffPlaceholder(temp,4); //the unsigned long
	
	msg_len = (int)retInfo.msglen;
	retInfo.msg = malloc( msg_len + 1);

	if(!fromServer){ //when server recieves from client
		
		char temp2[msg_len];
		memset(temp2, '\0', msg_len);
		for(int i = 12; i < (msg_len+12); i++){ //+12 offset to find msg

			temp2[i-12] = buffer[i];
		}
		strcpy(retInfo.msg, temp2);
		
	}
	else{ //when clients recieves msg from server

		int id_len = (int)retInfo.idLen;
		int nr_pads = getNrPads((int)msg_len);
		char id_buff[id_len];
		memset(id_buff, '\0', id_len);

		char temp2[msg_len];

		for(int i = 12; i < (msg_len+12); i++){ //+12 offset to find msg

			temp2[i-12] = buffer[i];
		}
		strcpy(retInfo.msg, temp2);

		//need to calc for pad.
		int lastElem = 12+msg_len+nr_pads;
		memset(temp2, '\0', msg_len);

		for(int i = lastElem; i< (lastElem+id_len); i++)
		{
			id_buff[i-lastElem] = buffer[i];
		}

		strcpy(retInfo.client_id, id_buff);
	}

	return retInfo;
}
/*
Simple function to calculate how much padding is put at the end of
a message in a MSG pdu. Used to know how much to change the offset of
cliend id message part in the buffer.
*/
int calculateMsgPad(int msgEndIndex){

	int padCount = msgEndIndex%4;
	int retVal = 0;

	switch(padCount){

		case 0:
			retVal = 0;
			break;
		case 1:
			retVal = 3;
			break;
		case 2:
			retVal = 2;
			break;
		case 3:
			retVal = 1;
			break;
	}

	return retVal;

}

/*
A function to read from one buffer to another from index start, to end.
Returns nothing since we use the argument buffmem as the buffer which we 
save infromation to.
*/
void readBOB(char buffer[],char buffmem[],int start, int end){ 
	
	int buffIndex = 0;
	for(int i = start; i<end; i++){

		buffmem[buffIndex] = (buffer[i]);
		buffIndex++;
	}
}

/*
A function to clear a buffer. Mostly used to clear tmp buffers used
in parse functions. Will be used after readBOB
*/
void clearBuffPlaceholder(char tmp[], int end){

	for(int i = 0; i < end; i++){
		tmp[i] = 0;
	}
}

/*
Small function to read a simple char byte at index, index of buffer.
returns the read character. 
*/
char readCharBytes(char buffer[],int index){

	return (buffer[index]); 
	//kan ändras så man tar bort 0b110000=48 så värdet bara är 1
}
int convert_ip(unsigned char n[4], char* ip)
{
    return sprintf(ip,"%d.%d.%d.%d",(int)n[0], (int)n[1], (int)n[2], (int)n[3]);
}


/*
Calculates the checksum over the whole message PDU

When the server calculates the recieved PDU it should get 255 on correct
messages.
Return: returns the checksum in form of a unsigned char. The checksum will only
be 0-255.
*/
unsigned char checkSumCalculator(unsigned char *pdu, int size){
    
	unsigned short checkSum = 0;
    for(int i = 0; i < size; i++){
        checkSum += (unsigned char)pdu[i];
    }

	checkSum = checkSum % 255;
	
   	checkSum = ~checkSum;
   	checkSum = checkSum & 0xFF;
	
	return (unsigned char)checkSum;

}
