#include "chatclient.h"




int main(int argc, char *argv[]){

	//check program requisits
    errArgc(argc);
    clientInfo ci;
    session sess;
    ci.ipv4[0] = '\0';
    serverList slist;
    int client_fd;

    getServerType(argv,&ci);

    if( -1 != setupClientConnection(&client_fd, ci) ){
	    
	    if(ci.serverFlag){
	    	sendGETLIST(&client_fd);
	  		readSLIST(&client_fd, &slist.si);
	   		chooseServerConnection(slist.si,&client_fd);
	    }

	   	if(-1 == sendJOIN(&client_fd, argv[1]) )
	   		printf(KRED"PJOIN: Failed to create a valid pdu "KNRM);
	   		
	   	
	   	sess.client_fd = &client_fd;
	   	strcpy(sess.client_id,argv[1]);
	   	memset(sess.buff,'\0',MAX_BUFF_SIZE);
	   	pthread_create( &sess.thread_id,NULL, &clientWriteEnd, &sess);
	   	
	   	whileOnline(&client_fd);
	   	pthread_join(sess.thread_id,NULL);
    }

    return 0;
}

/*
To check that correct number of arguments were specified to program.
If not a Usage of the function will be printed out.
Input: argc - number of arguments entered to program.
*/
void errArgc(int argc){


    if(argc != 5){
    
        printf("Usage:./chatclient identity ns/cs ns-hostname/cs-hostname");
        printf(" ns-portno/cs-portno\n\n");
        exit(-1);

    }
}

/*
Function: Gets the information from the server the user specified
as input to program. Information will be port nummber and host name.
*/
clientInfo getServerInfo(char *argv[]){

	struct clientInfo info; //chat server
	info.portno = 0;	


	int size = strlen(argv[4]);
	char tmp[size];

	for(int i = 0; i < size; i++){

		tmp[i] = argv[4][i]-48;
	}
	unsigned short tmpUS = convertArrayVal(tmp,size);

	info.portno = tmpUS;
	
	strcpy(info.hostname,argv[3]);

	return info;
}

/*
Function: Works like atoi but returns in type unsigned short.
Usage: only on string representations of numbers.
Returns: the unsigned short value of the numbers.
*/
unsigned short convertArrayVal(char arr[],int size){

	unsigned short r=0;
	int i;
	
	if(size == 1){
		r = powerOfTen(0) * (int)arr[0];
		return r;	
	}

	for(i = 0; i < size; i++){

		r += powerOfTen( size - (i+1) ) * (int)arr[i];
	}

	return r;
}

/*
Function: Help function to convert a string "1234" to a unsigned short 1234.
Returns: the unsigned short value of the numbers.
*/
unsigned short powerOfTen(int power){

	unsigned short num = 1;

	for(int i = 0; i < power; i++){
		
		num *= 10;

	}
	if(power == 0){
		num = 1;
	}

	return num;
}

/*
Function: Gets the information from the user input to know what type of
Returns 0 on success, else -1;
-1 indicates user typed in something that is not a chat server or name server.
*/
int getServerType(char *argv[],struct clientInfo *si){

	if(!strcmp("cs",argv[2]) ){

    	*si = getServerInfo(argv);
    	si->serverFlag = 0;
    }
    else if (!strcmp("ns",argv[2]) ){

    	*si = getServerInfo(argv);
    	si->serverFlag = 1;
    }
    else{
    	return -1;
    }
    return 0;
}

/*
Function: This function is used to set up a connection to a specified server
Input: clients socke file descriptor and clientInfo struct. clientInfo 
contains information of the server to connect.

returns 0 on succes and -1 on error.
*/
int setupClientConnection(int *fd, clientInfo si){

	
	char ip[50];
	host_to_ip(si.hostname, ip);

	//client_TCPsocket returns 0 success
	if( !client_TCPSocket(fd, si.portno, ip) ){ 
		printf("socket success!\n\n");
		return 0;
    }
    else{
    	printf("\nsocket fail!\n");
    	return -1;
    }
}






/*
Function: Small function to send GETLIST to the name server, will only be used
when ns is used as option in program.

Input: Client file descriptor
Output: Returns -1 on error, 0 on success.
-1 is a sign that we recieved invalid pdu. indicate this to client to resend
GETLIST and restart this process

*/

void sendGETLIST(int *client_fd){

	int byte_size = 0;

	char *getLIST = create_GETLIST( &byte_size);
	sendto_TCPSocket(client_fd,getLIST, byte_size);
}

/*
Small function to send GETLIST to the name server, will only be used
when ns is used as option in program.
*/
int readSLIST(int *client_fd, serverInfo *si){

	char buffer[MAX_BUFF_SIZE];
	int byte_count = 0;
	int test_acc = 0;

	// 0 < to stop on err
	byte_count = read_TCPSocket(client_fd,buffer, MAX_BUFF_SIZE);

	//validate_SLIST(int nr_bytes, char* pduMsg)
	test_acc = validate_SLIST(byte_count, buffer);

	if(test_acc){
		*si = parse_SLIST(buffer);
	}
	else{
		return -1;
	}
	
	for(int i = 0; i < si->nrOfServers; i++){
		printf("server: %s is open on port:%u\n",si->serv[i].servername,
				si->serv[i].portNum);

		printf("%s has %d nr of clients online\n\n",si->serv[i].servername,
								(unsigned char)si->serv[i].nrClients);
	}
	
	return 0;
}


/*
Function: This function is used when the client wants to join a new chat server.
It will send a JOIN pdu to the chat server to be registered.
returns 0 on SUCCESS, else -1.
*/
int sendJOIN(int *client_fd,char *buff){

	int byte_size = 0;
	pdu_JOIN j;
	j.identity = malloc(strlen(buff)+1);
	strcpy(j.identity,buff);
	j.identLen = strlen(j.identity);
	j.op_code = JOIN;

	char *joinBuff = create_JOIN(j,&byte_size);

	if( -1 != sendto_TCPSocket(client_fd,joinBuff,byte_size) ){
		free(joinBuff);
		free(j.identity);
		return 0;
	}
	free(j.identity);
	free(joinBuff);
	return -1;
}

/*
Function: This function is run during after a connection to a chat server
is established. It reads from the socket into a buffer with limit of 65535
bytes. Then reads which opcode it is and calls upon the right function to handle
the recieved message.

If something else but, PJOIN,PARTICIPANTS,MESS,QUIT and PLEAVE is recieved
connection is closed and program exists

Input: a pointer to the clients file descriptor for the socket.

*/
void whileOnline(int *client_fd){

	char buffer[MAX_BUFF_SIZE];
	memset(buffer,'\0',MAX_BUFF_SIZE);
	int byte_count = 0;
	int pdu = -1;
	int stop = 0;
	while(!stop){

		byte_count = read_TCPSocket(client_fd,buffer,MAX_BUFF_SIZE);
		if( 0 < byte_count){
			pdu = (int)buffer[0];
			switch(pdu){

				case PJOIN:
					casePJOIN(byte_count,buffer,*client_fd);
					break;

				case PARTICIPANTS:
					casePARTICIPANTS(byte_count,buffer,*client_fd);
					break;

				case MESS:
					caseMESS(byte_count,buffer,*client_fd);
					break;

				case PLEAVE:
					casePLEAVE(byte_count,buffer,*client_fd);
					break;

				case QUIT:
					caseQUIT(byte_count,buffer,*client_fd);
					stop = 1;
					break;

				default:
				close(*client_fd);
				stop = 1;

			}
		} else if(byte_count == 0)
		{
			stop = 1;
			close(*client_fd);
		}
		memset(buffer,'\0',MAX_BUFF_SIZE);
	}
}

/*
Function: This function handles when a participants pdu is sent to the client.
It will first validate the recieved PDU and then if its valid it will print
out all the participants of joined chat server.

Input:  number of read bytes, the buffer containing the message recieved
from the server and the file descriptor to close connection in case of
invalid PDU.

*/
void casePARTICIPANTS(int byte_count, char buff[],int fd){


	if(!validate_PARTICIPANTS(byte_count,buff) ){
		printf(KRED"not valid pdu!\n"KWHT);
		close(fd);
		exit(-1);
	}
	else{
		printf("participants case\n");	
		pdu_PARTICIPANTS *pa;
		pa = parse_PARTICIPANTS(buff);

		unsigned short n = (unsigned char)pa->numParticipants;
		printf("\nUsers online\n\n");
		for(int i = 0; i < n; i++){
			printf("[%s%s%s]\n", KGRN,pa->participants[i].name,KWHT);
			fflush(stdout);

		}
		free(pa);
		printf("\n\n");
	}
	
}

/*
Function: This function handles when QUIT is sent to the client.
It will tell the user that it recieved a QUIT and close connection.

Input:  number of read bytes, the buffer containing the message recieved
from the server and the file descriptor to close connection in case of
invalid PDU.
*/
void caseQUIT(int byte_count, char buff[], int fd){
		
	if(!validate_QUIT(byte_count, buff) ){
		printf(KRED"not valid pdu!\n"KWHT);
		close(fd);
		exit(-1);
	}

	printf(KRED"CLIENT RECIEVED A QUIT\n");
	printf(KRED"Server Shutting Down...\n"KNRM);
	close(fd);
}


/*
Function: This function handles when PLEAVE is sent to the client.
It will parse the message and tell the user which participant of the chat
has left and display date and time.

Input:  number of read bytes, the buffer containing the message recieved
from the server and the file descriptor to close connection in case of
invalid PDU.

*/
void casePLEAVE(int byte_count, char buff[], int fd){

	if(!validate_PJOIN_PLEAVE(byte_count, buff)){
		shutdown_TCPSocket(&fd);
	}
	else{
		pdu_PLEAVE *pl;
		pl = parse_PLEAVE(buff);
		printf("\n");
		epochConverter((time_t)pl->timestamp);
		fflush(stdout);
		printf("[%s%s%s] has %sleft%s the chat\n",
								KGRN,pl->identity,KWHT,KRED,KWHT);
		fflush(stdout);
	}
}



/*
Function: This function handles when MESS is recieved to the client from the
chat server and parses the message. If the message is not valid the connection 
will be closed and program exit.

*/
void caseMESS(int byte_count, char buff[], int fd){

	if(!validate_MESS(byte_count,buff)){
		printf(KRED"not valid pdu!\n"KWHT);
		close(fd);
		exit(-1);
	}
	else{

		pdu_MESS m;
		m = parse_MSG(buff,1);
		char newMess[(int)m.msglen];
		char newID[(int)m.idLen];
		filterMSG(m.msg, newMess,(int)m.msglen);
		filterMSG(m.client_id,newID,(int)m.idLen);
		printf("["KCYN"%s"KWHT"]:  "KYEL"%s"KWHT" ",newID,newMess);
		epochConverter((time_t)m.timestamp);
		fflush(stdout);
	
	}

}

/*
The recieved message and client name has to be filtered or else
junk will be printed out. This function is used after recieving a message.
*/
void filterMSG(char msg[], char filtered[],int msglen){

	int i;
	for( i = 0; i < msglen; i++){

		filtered[i] = msg[i];
	}
	filtered[i] = '\0';
}

void casePJOIN(int byte_count, char buff[],int fd){

	if(!validate_PJOIN_PLEAVE(byte_count, buff)){
		shutdown_TCPSocket(&fd);
	}
	else{
		pdu_PJOIN *pj;
		pj = parse_PJOIN(buff);
		
		epochConverter((time_t)pj->timestamp);
		printf("["KGRN"%s"KWHT"] has joined the chat\n",pj->identity);
		fflush(stdout);
		free(pj->identity);
		free(pj);
	}

}

int chooseServerConnection(serverInfo si,int *client_fd){

	printf("\nChoose a server to Connect to: \n");
	for(int i = 0; i <si.nrOfServers; i++){
		printf("%d:"KGRN" %s "KWHT"\n",i,si.serv[i].servername);

	}
	int val,valid = 0;

	while(!valid){

		printf("Server choice: ");
		scanf("%d",&val);
		fflush(stdout);
		if(val < 0 || val > si.nrOfServers){
			printf(KRED"Not a valid server choice!\n"KWHT);
		}
		else{
			valid = 1;
		}
	}

	clientInfo tmp;
	strcpy(tmp.hostname,si.serv[val].servername);

	strcpy( (char*)tmp.ipv4, (char*) si.serv[val].ipv4);
	tmp.portno = si.serv[val].portNum;

	return connectToSpecificServer(client_fd, tmp);
}


int connectToSpecificServer(int *fd, clientInfo si){

	printf("%s\n",si.ipv4);
	fflush(stdout);
	if( !client_TCPSocket(fd, si.portno,si.ipv4) ){
		printf("socket success!\n\n");
		return 0;
	}
	else{
		printf("\nsocket fail!\n");
		return -1;
	}	
}


/*
Epochconverter taken from internet
@http://www.epochconverter.com/programming/c
made small changes so it works with the client
*/
void epochConverter(time_t recvTime){

	struct tm  ts;
    char       buf[80];
    // Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
    ts = *localtime(&recvTime);
    strftime(buf, sizeof(buf), "[%a %Y-%m-%d %H:%M]\n", &ts);
    printf("%s", buf);
    fflush(stdout);
}

/*
The function that the writing thread runs. This function will be run as long
as the user wants to type and stay connected to the server. The user can
signal QUIT by pressing CTRL+D to initiate a QUIT send request.
Input: Session struct pointer which has information of the ongoing session.
*/
void *clientWriteEnd(void *sesh){

	session *sess = (session*)sesh;
	
	int size;
	sleep(1);
	while(1){
		
		if(!fd_is_valid(*sess->client_fd))
		{
			return NULL;
		}
		if( !fgets(sess->buff, MAX_BUFF_SIZE,stdin) ){
			sendQUIT(*sess);
			return NULL;
		}

		//Removes newline		
		sess->buff[strcspn(sess->buff, "\n")] = '\0';
		size = strlen(sess->buff);

		if(size > 0)
		{
			sendMESS(sess, sess->buff);
		}
		memset(sess->buff,'\0',MAX_BUFF_SIZE);
	}
}

/*
Function: This function is used when the client wants to send a Quit to
the chat server. If something goes wrong with sending the program closes and 
displays an error to the user.
Input: Session struct which has information of the ongoing session.
*/
void sendQUIT(session s){
	int unusedVar;
	//session *sess = s;
	char *quit = create_QUIT(&unusedVar);
	printf("User sent "KRED "Quit\n"KNRM);
	if(-1 == sendto_TCPSocket(s.client_fd,quit,4) ){
		printf(KRED"SEND ERROR\n"KWHT);
		free(quit);
		exit(-1);
	}
	free(quit);
}
/*
Function:This function Sends the written message by the user to the chat server.
If the message fails to send, the client closes the program and tells the
user something went wrong.
*/
void sendMESS(session *s, char *buff){

	fflush(stdout);
	pdu_MESS m;
	m.msg = buff;
	m.idLen = 0;
	int size,msglen;
	msglen = (int)strlen(buff);
	m.msglen = msglen;
	char *msgPDU;

	msgPDU = create_MESS(m,0,&size);

	fflush(stdout);	
	printf("\n");
	if( -1 == sendto_TCPSocket(s->client_fd,msgPDU,size) ){
		printf(KRED"SEND ERROR\n"KWHT);
		exit(-1);
	}

}

/*
Function: Used to strip off the \n character at the end of fgets.
the new value is saved through argument passed array.
*/
void rmNewLine(char scannedString[], char fixedString[], int size){

	int i;
	for( i = 0; i < size; i++){
	
		if( (int)scannedString[i] == '\n'){
			fixedString[i] = '\0';
			break;
		}
		
		fixedString[i] = scannedString[i];
	
	}
	
}

