#include "socket_lib.h"


/* 
*	Args: socket_filedescriptor, portnumber to bind.
*	Returns 0 on success, -1 on failure.
*/
int create_UDPSocket(int *socket_fd, int portno)
{
	struct sockaddr_in s_me;
	int opt = 1;

	if((*socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return -1;

	memset((char *) &s_me, 0, sizeof(s_me));

	s_me.sin_family = AF_INET;
	s_me.sin_port = htons(portno);
	s_me.sin_addr.s_addr = htonl(INADDR_ANY);

	setsockopt(*socket_fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt, sizeof(int));

	if(bind(*socket_fd, (struct sockaddr *)&s_me, sizeof(s_me)) == -1)
		return -1;

	return 0;
}

/*
* Returns -1 on failure, otherwise nr of bytes received. 
* Received msg will be stored in buffer
* The expected IP is the ip of the nameserver.
*/
int read_UDPSocket(int *socket_fd, char* buffer, char* expected_ip, int buf_size)
{
	struct sockaddr_in s_nameserver;
	unsigned int len = sizeof(s_nameserver);

	memset((char *) &s_nameserver, 0, sizeof(s_nameserver));
	int nr_bytes = 0;

	nr_bytes = recvfrom(*socket_fd, buffer, buf_size, 0,
						(struct sockaddr *) &s_nameserver, &len);

	if(strcmp(expected_ip, inet_ntoa(s_nameserver.sin_addr)) != 0)
		return -1;

	return nr_bytes;
}

/*
*	Sends the msg to ip_addr on port_no. 
*	Returns 0 on success, -1 on failure. 
*/
int sendto_UDPSocket(int *socket_fd, int portno, char *ip, char* msg, int msg_bytes)
{
	struct sockaddr_in s_other;
	memset((char *) &s_other, 0, sizeof(s_other));

	unsigned int len = sizeof(s_other);

	memset((char *) &s_other, 0, sizeof(s_other));
	s_other.sin_family = AF_INET;
	s_other.sin_port = htons(portno);
	s_other.sin_addr.s_addr = inet_addr(ip);

	if(sendto(*socket_fd, msg, msg_bytes,0, (struct sockaddr*) &s_other, len) == -1)
		return -1;
	return 0;
}

int close_UDPSocket(int *socket_fd)
{
	close(*socket_fd);
	return 1;
}


/*
* Creates a TCP socket used on the server that clients connect to. 
* Returns 0 on success, -1 on failure. The file descriptor 
* used to communicate with the other part will be pointed to
* by socket_fd
*/
int server_TCPSocket(int *serv_fd, int portno, char* ip)
{
	struct sockaddr_in s_this; 

	if((*serv_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		return -1;

	s_this.sin_family = AF_INET;
	s_this.sin_port = htons(portno);
	s_this.sin_addr.s_addr = inet_addr(ip);

	memset(s_this.sin_zero, '\0', sizeof(s_this.sin_zero));

	int optval = 1;
	setsockopt(*serv_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	bind(*serv_fd, (struct sockaddr *) &s_this, sizeof(s_this));

	/*Max 2 connection queued*/
	//listen(serv_fd, 3);
	//*socket_fd = accept(temp_fd,(struct sockaddr *)&s_other, &len);

	
	return 0;
}

/*
* Creates the client version of the TCP-socket. 
* Socket_fd will point to the new filedescriptor
*/
int client_TCPSocket(int *socket_fd, int portno, char* ip)
{
	struct sockaddr_in s_serv;
	socklen_t len = sizeof(s_serv);

	*socket_fd = socket(PF_INET, SOCK_STREAM, 0);

	s_serv.sin_family = AF_INET;
	s_serv.sin_port = htons(portno);
	s_serv.sin_addr.s_addr = inet_addr(ip);

	memset(s_serv.sin_zero, '\0', sizeof(s_serv.sin_zero));
	if(connect(*socket_fd, (struct sockaddr * ) &s_serv, len) != 0)
		return -1;

	return 0;
}

/*Returns -1 on error*/
int read_TCPSocket(int *socket_fd, char* buf, unsigned int buf_size)
{
	return recv(*socket_fd, buf, buf_size, 0);
}


/*Returns -1 on error, otherwise nr of bytes sent. */
int sendto_TCPSocket(int *socket_fd, char* buf, unsigned int buf_size)
{
	if(!fd_is_valid(*socket_fd))
	{
		return -1;
	}

	return send(*socket_fd, buf, buf_size, 0);
}

/*Shuts down the socket, TODO -> Test this for correct flag
* Returns -1 on error, 0 on success
*/
int shutdown_TCPSocket(int *socket_fd)
{
	/*SHUT_RDWR will stop socket from both receiving and sending*/
	return shutdown(*socket_fd, SHUT_RDWR);
}

/*
*This function takes a hostname and tries to get the IP 
* that matches the hostname. If successfull the IP will be saved
* in the char pointer ip.
* Return: 0 on success, 1 if no address could be found.
*/
int host_to_ip(char *hostname, char *ip)
{
	struct hostent *he;
	struct in_addr **addresses;

	if((he = gethostbyname(hostname)) == NULL)
	{
		return 1;
	}

	addresses = (struct in_addr **) he->h_addr_list;

	for(int i = 0; addresses[i] != NULL; i++)
	{
		strcpy(ip, inet_ntoa(*addresses[i]));
		return 0;
	}

	return 1;
}

/*Checks whether an FD has closed*/
int fd_is_valid(int fd)
{
    return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}



/*
This function will keep reading exactly the specified nr of bytes given
from the input. After it is done reading the exact nr of bytes it will return
the buffer which it read into

Input: the file descriptor to the socket, number of bytes to read and a poitner
to how many bytes are read so that the read number of bytes can be returned 
through the pointer

Return: The read message recieved from the socket.

*/
char *readExactly(int fd, char *buf, int bytes_to_read, int *bytes_read){
	
	char *tmpBuf;

	tmpBuf = (char *)realloc(buf,bytes_to_read+10);


	do{
		//keep on reading into same buffer at the same index as we stopped at
		*bytes_read += read_TCPSocket(&fd,&tmpBuf[*bytes_read], bytes_to_read);

	}while(*bytes_read <= bytes_to_read);
	
	for(int i = 0; i <*bytes_read; i++){
		printf("%d ",(unsigned char)tmpBuf[i] );
	}

	fflush(stdout);
	return tmpBuf;
}

