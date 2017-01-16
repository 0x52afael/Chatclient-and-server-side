FLAGS=-Wall -g -std=c99
THREADS = -pthread
CC=gcc
EXTRA= -D_POSIX_SOURCE -D_GNU_SOURCE -pthread

all: test server client valid testcli failpdu

failpdu: pdu.o socket_lib.o pdu_validator.o failpdu.o 
	$(CC) $(FLAGS) $(EXTRA) failpdu.o pdu.o socket_lib.o pdu_validator.o -o failpdu

testcli: pdu.o socket_lib.o pdu_validator.o tc.o 
	$(CC) $(FLAGS) $(EXTRA) tc.o pdu.o socket_lib.o pdu_validator.o -o testclient

client: pdu.o socket_lib.o pdu_validator.o cc.o
	$(CC) $(FLAGS) $(EXTRA) cc.o pdu.o socket_lib.o pdu_validator.o -o client

cc.o: chatclient.c chatclient.h
	$(CC) $(FLAGS) -c chatclient.c -o cc.o

failpdu.o:
	$(CC) $(FLAGS) -c failpdu_client.c -o failpdu.o

tc.o: 
	$(CC) $(FLAGS) -c testClient.c -o tc.o

server: pdu.o chat_server.o socket_lib.o pdu_validator.o msgqueue.o
	$(CC) $(FLAGS) $(THREADS) chat_server.o pdu.o socket_lib.o pdu_validator.o msgqueue.o -o server

msgqueue.o: msgqueue.c msgqueue.h
	$(CC) $(FLAGS) -c msgqueue.c -o msgqueue.o

chat_server.o: chat_server.c chat_server.h
	$(CC) $(FLAGS) $(THREADS) -c chat_server.c -o chat_server.o

socket_lib.o: socket_lib.c socket_lib.h
	$(CC) $(FLAGS) -c socket_lib.c -o socket_lib.o

valid: pdu_validator.o
	$(CC) $(FLAGS) pdu_validator.o -o valid

pdu_validator.o: pdu_validator.c pdu_validator.h
	$(CC) $(FLAGS) -c pdu_validator.c -o pdu_validator.o

test: test.o pdu.o
	$(CC) $(FLAGS) test.o pdu.o -o test

test.o: testsuite.c testsuite.h
	$(CC) $(FLAGS) -c testsuite.c -o test.o

pdu.o: PDU.c PDU.h
	 $(CC) $(FLAGS) -c PDU.c -o pdu.o
clean:
	rm -f *.o pdu test server client
