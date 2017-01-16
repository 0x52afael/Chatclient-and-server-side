#include "chat_server.h"


void* client_thread_func(void* cli);



/*Adds a client to the client list*/
/*THREADSAFE*/
int append_Clientlist(client* c)
{
	pthread_mutex_lock(&clientList_LOCK);
	for(int i = 0; i<255; i++)
	{
		if(clientList[i].idNum == 0)
		{
			c->idNum = i+1;
			clientList[i] = *c;
			pthread_mutex_unlock(&clientList_LOCK);
			return 0;
		}
	}

	pthread_mutex_unlock(&clientList_LOCK);
	return 1;
}

/*Checks whether a client has sent the initial JOIN
 or not, we dont allow other PDUs before that.
 Return 1 = has joined, 0 = has not joined
 	THREAD SAFE
 */
int clientHasJoined(int id)
{
	pthread_mutex_lock(&clientList_LOCK);
	for(int i = 0; i<255; i++)
	{
		if(clientList[i].idNum == id)
		{
			pthread_mutex_unlock(&clientList_LOCK);
			return clientList[i].hasJoined;
		}
	}
	pthread_mutex_unlock(&clientList_LOCK);
	return 0;
}

/*
* Checks if a username is already taken.
* Returns true if taken, false if not. 
* THREADSAFE
*/
int userNameExists(char *userName)
{
	pthread_mutex_lock(&clientList_LOCK);
	for(int i = 0; i<255; i++)
	{
		if(strcmp(clientList[i].name, userName) == 0)
		{
			pthread_mutex_unlock(&clientList_LOCK);
			return 1;
		}
	}
	pthread_mutex_unlock(&clientList_LOCK);
	return 0;
}

/*Sets a client as joined
 THREADSAFE*/
void setHasJoined(int id)
{
	pthread_mutex_lock(&clientList_LOCK);
	for(int i = 0; i<255; i++)
	{
		if(clientList[i].idNum == id)
		{
			clientList[i].hasJoined = 1;
			break;
		}
	}
	pthread_mutex_unlock(&clientList_LOCK);
}

/*Inits the client list*/
/*THREADSAFE*/
void init_ClientList()
{
	pthread_mutex_lock(&clientList_LOCK);
	for(int i = 0; i<255; i++)
	{
		clientList[i].idNum = 0;
		clientList[i].hasJoined = 0;
		clientList[i].fd = 0;
	}
	pthread_mutex_unlock(&clientList_LOCK);
}

/*Removes a client by client ID*/
/*Returns 0 if client was removed, -1 if not found*/
/*THREADSAFE*/
int remove_Client(int id_num)
{
	pthread_mutex_lock(&clientList_LOCK);
	for(int i = 0; i<255; i++)
	{
		if(clientList[i].idNum == id_num)
		{
			clientList[i].idNum = 0;
			clientList[i].hasJoined = 0;
			close(clientList[i].fd);
			memset(clientList[i].name, '\0' , 255);
			clientList[i].fd = 0;
			pthread_mutex_unlock(&clientList_LOCK);
			return 0;
		}
	}
	pthread_mutex_unlock(&clientList_LOCK);
	return -1;
}

/*Returns number of connected clients*/
/*THREADSAFE*/
int size_ClientList()
{
	int nr = 0;

	pthread_mutex_lock(&clientList_LOCK);
	for(int i = 0; i< 255; i++)
	{
		if(clientList[i].idNum != 0)
			nr++;
	}
	pthread_mutex_unlock(&clientList_LOCK);

	return nr;
}

/*
* Registers the server to the name-server using a UDP connection.
* If the response is incorrect or it does not succeed in connecting, 
* it will try again. 
*/
int register_Server(int* fd, int* idNum, connectionInfo *ci)
{
	char ip_address[50];
	char buf[128];
	int mem_size, ret;
	struct pollfd pfd;

	pfd.fd = *fd;
	pfd.events = POLLIN;

	pdu_REG pdu;
	pdu.op_code = REG;
	pdu.portnum = ci->myPort;
	strcpy(pdu.servName, ci->myName);
	pdu.servNL = strlen(pdu.servName);

	host_to_ip(ci->hostName, ip_address);
	char *msg = create_REG(pdu, &mem_size);
	memset(buf, '\0', 128);

	/*Try connecting forever, clients can still use the server anyway*/
	for(;;)
	{
		printf("Connecting to nameserver... ");
		sendto_UDPSocket(fd, ci->nsPort, ip_address, msg, mem_size);

		ret = poll(&pfd, 1, 5000); //Wait for 5 seconds
//
		switch(ret)
		{
			case 0:
				printf("didnt get a response in 5 seconds, trying again\n");
				continue;
			case -1:
				perror("poll()");
				free(msg);
				exit(1);
			default:
				read_UDPSocket(fd, buf, ip_address, 128);
		}

		if(buf[0] != ACK)
		{
			printf("got incorrect response, trying again.\n");
			continue;
		}

		pdu_ACK* ack = parse_ACK(buf);
		*idNum = ack->identityNum;
		free(ack);
		printf("Server registered, id-num %d\n", *idNum);
		break;
	}

	free(msg);

	return 0;
}

/*Sends an ALIVE pdu to the server via UDP.
  Returns: -1 if we didnt get a response
			-2 if server is not registered
			0 if everything went correctly.

 */
int server_Alive(int* fd, char nrClients, unsigned short idNum)
{
	struct pollfd pfd;
	char ip_address[50];
	char buf[128];
	int mem_size, ret;

	pfd.fd = *fd;
	pfd.events = POLLIN;
	host_to_ip("itchy.cs.umu.se", ip_address);

	pdu_ALIVE aliv;
	aliv.op_code = ALIVE;
	aliv.nrOfClients = nrClients;
	aliv.identityNum = idNum;

	char* msg = create_ALIVE(aliv, &mem_size);
	if(sendto_UDPSocket(fd, 1337, ip_address, msg, mem_size) == -1)
		return -1;
	
	ret = poll(&pfd, 1, 5000); //Wait for 5 seconds
	
	switch(ret)
	{
		case 0:
			return -1;
		case -1:
			perror("poll");
			free(msg);
			exit(2);
		default:
			if(read_UDPSocket(fd, buf, ip_address, 128) == -1)
			{
				free(msg);
				return -1;
			}
	}

	free(msg);
	return (buf[0] == ACK ? 0 : -2); 
}

/*
* Keeps the server registered to the nameserver
* by sending an alive PDU every five seconds. 
* If the server has lost registration it will re-register.
*/
void alive_thread_func(void* cInfo)
{
	int fd = 0, idNum = 0, nrClients = 0, ret = 0;
	connectionInfo *ci = NULL;
	ci = (connectionInfo *) cInfo;

	create_UDPSocket(&fd, ci->myPort);
	register_Server(&fd, &idNum, ci);

	while(1)
	{
		nrClients = size_ClientList(); 
		ret = server_Alive(&fd, nrClients, (unsigned short) idNum);
		
		switch(ret)
		{
			case 0:
				break;
			/*If we didnt get a response sleep for a sec and try again*/
			case -1:
				sleep(1);
				continue;
			default:
				register_Server(&fd, &idNum, ci);
		}
	
		sleep(4);
	}

}

/*
* This function will be used by a dedicated thread that
* listens to the given port number for TCP connections.
* When a TCP connection comes it will spawn a new thread that
* will represent the new client. 
*/
void* listenForClients(void *cInfo)
{
	int serv_fd, nr_clients;
	socklen_t len = sizeof(struct sockaddr_in);
	char serv_ip[50], myHostName[1024];

	connectionInfo *ci = (connectionInfo *) cInfo;
	
	if(gethostname(myHostName, 1023) == -1)
	{
		fprintf(stderr, "Could not get hostname of this computer, exiting..\n");
		exit(7);
	}

	host_to_ip(myHostName, serv_ip);
	server_TCPSocket(&serv_fd, ci->myPort, serv_ip);

	while(1)
	{		
		/*Allow 4 queued connections*/
		listen(serv_fd, 5);
		nr_clients = size_ClientList();
		if(nr_clients >= 255)
		{
			usleep(100000); //sleep 100 ms, no point in instantly retrying
			continue;
		}

		struct sockaddr_in s_client;
		client *c = malloc(sizeof(struct client)); 
		c->fd = accept(serv_fd, (struct sockaddr *)&s_client, &len);
		c->idNum = 0;
		c->hasJoined = 0;

		pthread_t new_Client;
		if(pthread_create(&new_Client, NULL, client_thread_func, c))
		{
			perror("pthread_create");
			exit(5);
		}

	}
}

/*
* Sends participants to the newly connected client.
*/
void sendParticipants(int* fd)
{
	int byte_size, nr_participants = 0;
	unsigned short name_lengths = 0;
	pdu_PARTICIPANTS p;
	p.op_code = PARTICIPANTS;
	char *msg;

	pthread_mutex_lock(&clientList_LOCK);
	for(int i = 0; i<255; i++)
	{
		if(clientList[i].idNum == 0)
			continue;
		nr_participants++;
		name_lengths += strlen(clientList[i].name);
	}

	p.id_len = name_lengths + nr_participants;
	p.numParticipants = nr_participants;
	msg = create_PARTICIPANTS(p, clientList, &byte_size);
	pthread_mutex_unlock(&clientList_LOCK);

	if(!validate_PARTICIPANTS(byte_size, msg))
	{
		printf(" PARTICIPANTS CREATED NOT VALID!\n");
	} else
	{
		sendto_TCPSocket(fd, msg, (unsigned int) byte_size);
		free(msg);	
	}
}

/*Removes the client from the clientlist and exits the thread*/
void killConnection(client *c)
{
	remove_Client(c->idNum);
	pthread_exit(NULL);
}

/*
* Used when server recieves a JOIN message from a client.
* Creates the PJOIN PDU and sends to everyone but the new
* client.
*/
void handleJOIN(char* buf, client *c)
{
	int byte_size = 0;
	pdu_JOIN* pdu = parse_JOIN(buf);

	pdu_PJOIN pj;
	pj.op_code = PJOIN;
	pj.identLen = pdu->identLen;
	pj.identity = strdup(pdu->identity); //DUP not CPY, 
	pj.timestamp = (unsigned long) time(NULL);

	char *join_msg = create_PJOIN(pj, &byte_size);

	pthread_mutex_lock(&clientList_LOCK);
	for(int i = 0; i<255; i++)
	{
		if(clientList[i].idNum != 0)
		{
			sendto_TCPSocket(&(clientList[i].fd), join_msg, (unsigned int) byte_size);
		}
	}
	pthread_mutex_unlock(&clientList_LOCK);
	
	c->hasJoined = 1;
	strcpy(c->name, pdu->identity);
	append_Clientlist(c);
	setHasJoined(c->idNum);
	sendParticipants(&(c->fd));

	free(join_msg);
	free(pj.identity);
	free(pdu->identity);
	free(pdu);
}

/*
* Used when server recieves a QUIT PDU.
* Creates a PLEAVE PDU and adds it to the message queue.
*/
void handleQUIT(client *c)
{
	pdu_PJOIN pdu; /*PJOIN and PLEAVE contains the exact same elements*/
	int byte_size = 0;

	pdu.op_code = PLEAVE;
	pdu.identLen = strlen(c->name);
	pdu.timestamp = (unsigned long) time(NULL);
	pdu.identity = strdup(c->name);

	QueueItem qi;
	qi.msg = create_PLEAVE(pdu, &byte_size);
	qi.byte_size = byte_size;

	remove_Client(c->idNum);

	pthread_mutex_lock(&messageQueue_LOCK);
	msgQueue_add(&messageQueue, &qi, sizeof(qi));
	pthread_mutex_unlock(&messageQueue_LOCK);

	free(pdu.identity);
	free(c);
	pthread_exit(NULL);
}


/*
* Sends a QUIT PDU a leaving client.
*/
void sendQUIT(client *c)
{
	int byte_size = 0;
	char* leave_msg = create_QUIT(&byte_size);

	sendto_TCPSocket(&(c->fd), leave_msg, (unsigned int) byte_size);
	
	handleQUIT(c); // This will send PLEAVE to the rest of clients and remove
					// c from the list.

	free(leave_msg);	
}

/*
* Used when server recieves a message PDU.
* A new PDU is created containing relevant information
* and added to the message queue.
*/
void handleMESS(char *buf, client *c)
{
	int byte_size = 0;
	pdu_MESS pdu;

	pdu = parse_MSG(buf, 0);
	
	pdu.timestamp = (unsigned long) time(NULL);
	strcpy(pdu.client_id, c->name);
	pdu.idLen = strlen(pdu.client_id);

	QueueItem qi;
	qi.msg = create_MESS(pdu, 1, &byte_size);

	qi.byte_size = byte_size;

	pthread_mutex_lock(&messageQueue_LOCK);
	msgQueue_add(&messageQueue, &qi, sizeof(qi));
	pthread_mutex_unlock(&messageQueue_LOCK);

	free(pdu.msg);
}


/*
* Checks that the client ID of a connecting client is not
* already taken.
*/
void checkUserName(char *buf, client *c)
{
	int byte_size = 0;
	pdu_JOIN *pdu = parse_JOIN(buf);

	if(userNameExists(pdu->identity))
	{
		char *str = "Username Already Taken";
		pdu_MESS mess;
		mess.msg = strdup(str);
		mess.msglen = strlen(str);
		strcpy(mess.client_id, pdu->identity);
		mess.idLen = strlen(mess.client_id);

		char *msg = create_MESS(mess, 1, &byte_size);
		sendto_TCPSocket(&(c->fd), msg, (unsigned int) byte_size);
		char *quit = create_QUIT(&byte_size);
		sendto_TCPSocket(&(c->fd), quit, (unsigned int) byte_size);

		free(pdu->identity);
		free(pdu);
		free(mess.msg);
		free(msg);
		free(quit);
		free(c);
		pthread_exit(NULL);
	}

	free(pdu->identity);
	free(pdu);
}

/*
* When a new message is read on a socket
* it is saved in a buffer and sent to this function.
* It will handle the message depending on the OP_CODE
*/
void handleMessage(int nr_bytes, char *buf, client *c)
{
	char b = buf[0];

	switch(b)
	{
		case JOIN:
			printf("here?\n");
			if(!validate_JOIN(c->totalRead, buf) || clientHasJoined(c->idNum))
			{
				sendQUIT(c);
				break;
			}
			checkUserName(buf, c);
			handleJOIN(buf, c);
			break;
			
		case QUIT:
			handleQUIT(c);
			break;

		case MESS:
			if(!validate_MESS(c->totalRead, buf) || !c->hasJoined)
			{
				sendQUIT(c);
				break;
			}
			handleMESS(buf, c);
			break;	

		default:
			printf("gets here5?\n");
			sendQUIT(c); //due to incorrect pdu
	}
}

/*
* Function used by client threads. Will
* read the socket and handle the recieved PDUs.
* Thread exits itself when connection is closed. 
*/
void* client_thread_func(void* cli)
{
	int nr_bytes = 0, ret = 0;
	client *c = NULL;
	c = (client *) cli;

	struct pollfd pfd;
	pfd.fd = c->fd;
	pfd.events = POLLIN;

	while(1)
	{
		char *buf = calloc(1,1);
		if(!fd_is_valid(c->fd))
			handleQUIT(c);
			
		

		ret = poll(&pfd, 1, 5000); //Wait for 5 seconds
		switch(ret)
		{
			case 0:
				continue;
			case -1:
				perror("poll()");
				exit(1);
			default:
				break;
		}

		nr_bytes = read_TCPSocket(&(c->fd), buf, 1);
		
		buf = handleByteStream(buf,c,&nr_bytes);
		if(nr_bytes == 0)
		{
			printf("server read 0 bytes\n");
			if(c->hasJoined)
				handleQUIT(c);
			
			//Exit thread
			killConnection(c);
		}
		else{
			handleMessage(nr_bytes, buf, c);
			
			
		}
		free(buf);
	}
}

/*
* Used by the sending thread to send a message
* to the entire clientlist.
*/
void sendMessage(char *msg, int byte_size)
{
	pthread_mutex_lock(&clientList_LOCK);
	for(int i = 0; i<255; i++)
	{
		if(clientList[i].idNum != 0)
		{
			sendto_TCPSocket(&(clientList[i].fd), msg, (unsigned int) byte_size);
		}
	}
	pthread_mutex_unlock(&clientList_LOCK);

}

/*
* Sending threads function. Will try to
* pop a message from the message queue and 
* send it to everyone.
*/
void* sender_thread_func()
{
	int i = 0;
	while(1)
	{
		pthread_mutex_lock(&messageQueue_LOCK);
		i = msgQueue_length(&messageQueue);
		if(i > 0)
		{
			void *data = NULL;
			msgQueue_pop(&messageQueue, &data);
			QueueItem *qi = NULL;
			qi = (QueueItem*)data;
			sendMessage(qi->msg, qi->byte_size);
			free(qi->msg);
			free(qi);
		}
		pthread_mutex_unlock(&messageQueue_LOCK);

		usleep(5000); //Sleep 5 ms to try avoid constantly locking the queue
	}
}

/*Initiate locks*/
void initMutexes()
{
	if(pthread_mutex_init(&clientList_LOCK, NULL) != 0)
	{
		perror("mutex clientlist failed");
		exit(6);
	}
	if(pthread_mutex_init(&messageQueue_LOCK, NULL) != 0)
	{
		perror("mutex msgQueue failed");
		exit(6);
	}
}

/*Starts the nameserver thread, TCP-listen thread and sender-thread*/
void startThreads(connectionInfo *ci)
{
	pthread_t nameServerThread, clientListenThread, senderThread;

	if(pthread_create(&nameServerThread, NULL, (void*)alive_thread_func, ci))
	{
		perror("Couldnt create name server thread");
		exit(3);
	}
	if(pthread_create(&clientListenThread, NULL, listenForClients, ci))
	{
		perror("Couldnt create tcp listen thread");
		exit(3);
	}
	if(pthread_create(&senderThread, NULL, sender_thread_func, NULL))
	{
		perror("Couldnt create sender thread");
		exit(3);
	}

}

void initQueues()
{
	msgQueue_new(&messageQueue, NULL);
}

/*
* Parses the arguments given by the user when program
* is started and saves it in a struct.
*/
void initArguments(int argc, char *argv[], connectionInfo *ci)
{
	char *usage = "Usage: [PORT] [SERVERNAME] [NAMESERVER-HOST] [NAMESERVER-PORT]\n";

	if(argc != 5)
	{
		fprintf(stderr, "%s", usage);
		exit(5);
	}
	
	if((ci->myPort = (int)strtol(argv[1], NULL, 10)) == 0)
	{
		fprintf(stderr, "Invalid port [%s]\n", argv[1]);
		fprintf(stderr, "%s", usage);
	}

	if(strlen(argv[2]) > 255)
	{
		fprintf(stderr, "SERVERNAME too big, maximum name length is 255, was %d\n", 
															 (int)strlen(argv[2]));
	}

	if(strlen(argv[3]) > 255)
	{
		fprintf(stderr, "NAMESERVER-HOST too big, maximum name length is 255, was %d\n", 
																   (int)strlen(argv[3]));

	}
	strcpy(ci->myName, argv[2]);
	strcpy(ci->hostName, argv[3]);

	if((ci->nsPort = (int)strtol(argv[4], NULL, 10)) == 0)
	{
		fprintf(stderr, "Invalid port [%s]\n", argv[4]);
		fprintf(stderr, "%s", usage);
	}
}


/*
This function will handle byte streams that are 1 byte at a time.
Input:
Return:
*/

/*

int msg_pads = getNrPads((int)m.msglen);
int mem_size = (int)m.msglen + msg_pads + 12;

*/
char *handleByteStream(char *pdu, client *c, int *bytes_read){

	int bytes_to_read = 0;
	int index = 1;
	unsigned short msgLen = 0;
	char *head;
	char *body;
	switch( (int)pdu[0] ){

		case MESS:

			c->totalRead = 0;
			*bytes_read += 6; //read entire header + 4 bytes for message len
							//and pads that come after msg len

			head = readExactly(c->fd,pdu, *bytes_read,&index); 
			*bytes_read += 1;//to index to next slot to save to

			memcpy(&msgLen, &pdu[4], sizeof(unsigned short));
			msgLen = ntohs(msgLen);
			bytes_to_read += 4+msgLen;
			bytes_to_read += getNrPads(msgLen);
			c->totalRead = bytes_to_read+(*bytes_read);
			body = readExactly(c->fd,head, bytes_to_read,bytes_read); 

		break;

		case JOIN:

			c->totalRead = 0;
			*bytes_read += 2; //read entire header 
			head = readExactly(c->fd,pdu, *bytes_read,&index); 
			*bytes_read += 1; //to index to next slot to save to
			bytes_to_read = (unsigned char)pdu[1];
			bytes_to_read += getNrPads((unsigned char)pdu[1]);
			c->totalRead += bytes_to_read+(*bytes_read);
			body = readExactly(c->fd,head, bytes_to_read,bytes_read); 


		break;

		case QUIT:
			c->totalRead = 0;
			*bytes_read = 3;
			body = readExactly(c->fd,pdu, *bytes_read,&index); 
			c->totalRead = 4;
		break;
	}

	return body;

}



int main(int argc, char *argv[])
{
	connectionInfo ci;

	initArguments(argc, argv, &ci);
	initQueues();
	initMutexes();
	init_ClientList();
	startThreads(&ci);


	while(1)
	{
		sleep(3);
	}

	return 0;
}


