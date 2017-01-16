#include "testsuite.h"

int main(void)
{
	setlocale(LC_ALL, "en_US.utf8"); //vart ska man göra detta?
	printf("Starting Tests for PDU..\n");
	

	printf("test_pdu_MSG.. ");
	test_pdu_MSG();
	printf("OK\n");

	printf("test_pdu_NOTREG.. ");
	test_pdu_NOTREG();
	printf("OK\n");

	printf("test_pdu_QUIT.. ");
	test_pdu_QUIT();
	printf("OK\n");

	printf("test_pdu_SLIST.. ");
	test_pdu_SLIST();
	printf("OK\n");

	printf("test_pdu_REG.. ");
	test_pdu_REG();
	printf("OK\n");
	
	printf("test_pdu_ACK.. ");
	test_pdu_ACK();
	printf("OK\n");
	
	printf("test_pdu_JOIN.. ");
	test_pdu_JOIN();
	printf("OK\n");

	printf("test_pdu_PJOIN.. ");
	test_pdu_PJOIN();
	printf("OK\n");

	printf("test_pdu_PARTICIPANTS.. ");
	test_pdu_PARTICIPANTS();
	printf("OK\n");

	printf("test_pdu_PLEAVE.. ");
	test_pdu_PLEAVE();
	printf("OK\n");

	

	return 0;
}

/*
* Test function for pdu_REG. Used to see if the given input to the
* function will return the expected output.
*/
int test_pdu_REG()
{
	int unused;
	pdu_REG r;
	r.op_code = REG;
	r.portnum = 5000;
	sprintf(r.servName, "HELLOWORLD");
	r.servNL = strlen(r.servName);
	char* reg_msg = create_REG(r, &unused);

	pdu_REG *pdu_info = parse_REG(reg_msg);

	assert(r.op_code == pdu_info->op_code);
	assert(5000 == (int)pdu_info->portnum);
	assert(10 == (int)pdu_info->servNL);
	assert(!strcmp(r.servName, pdu_info->servName));

	free(reg_msg);
	free(pdu_info);

	return 1;
}
/*
* Test function for pdu_ACK to see if the given input to the
* function will return the expected output.
*/
int test_pdu_ACK()
{
	int unused;
	pdu_ACK a;
	a.op_code = ACK;
	a.identityNum = 55555;
	char* ack_msg = create_ACK(a, &unused);

	pdu_ACK *pdu_info = parse_ACK(ack_msg);

	assert(ACK == (int)pdu_info->op_code);
	assert(a.identityNum == pdu_info->identityNum);

	free(ack_msg);
	free(pdu_info);

	return 1;
}
/*
* Test function for pdu_JOIN to see if the given input to the
* function will return the expected output.
*/
int test_pdu_JOIN()
{
	int unused;
	pdu_JOIN j;
	j.identity = "THIS IS MY IDENTITY123";
	j.identLen = strlen(j.identity);
	j.op_code = JOIN;

	char *join_MSG = create_JOIN(j, &unused);
	pdu_JOIN *pdu_info = parse_JOIN(join_MSG);

	assert(j.op_code == ((int)pdu_info->op_code) );
	assert(j.identLen == pdu_info->identLen);
	assert(!strcmp(j.identity, pdu_info->identity) );

	free(pdu_info->identity);
	free(pdu_info);
	free(join_MSG);

	return 1;
}
/*
* Test function for pdu_PJOIN to see if the given input to the
* function will return the expected output.
*/
int test_pdu_PJOIN()
{
	int unused;
	pdu_PJOIN pj;
	pj.identity = "THIS IS MY IDENTITY123";
	pj.identLen = strlen(pj.identity);
	pj.timestamp = time(NULL);

	char *msg = create_PJOIN(pj, &unused);
	
	pdu_PJOIN *pdu = parse_PJOIN(msg);

	assert(PJOIN == (int)pdu->op_code);
	assert(pj.identLen == (int)pdu->identLen);
	assert(pj.timestamp == pdu->timestamp);
	assert(!strcmp(pj.identity, pdu->identity) );

	free(pdu->identity);
	free(pdu);
	free(msg);

	return 1;
}

int test_pdu_PARTICIPANTS()
{
	int unused;
	pdu_PARTICIPANTS pa;
	pa.op_code = PARTICIPANTS;
	pa.numParticipants = 3;

	client arr[3];
	arr[0].idNum = 1;
	arr[1].idNum = 2;
	arr[2].idNum = 3;
	
	strcpy(arr[0].name, "Klient 1");
	strcpy(arr[1].name, "Klient 2");
	strcpy(arr[2].name, "Klient 3");

	pa.id_len = strlen(arr[0].name) + strlen(arr[1].name) + strlen(arr[2].name);

	char *msg = create_PARTICIPANTS(pa, arr,  &unused);
	pdu_PARTICIPANTS *pdu_info = parse_PARTICIPANTS(msg);

	assert(pa.op_code == pdu_info->op_code);
	assert(pa.numParticipants == pdu_info->numParticipants);
	assert(pa.id_len == pdu_info->id_len);
	assert(!strcmp(pdu_info->participants[0].name, "Klient 1"));
	assert(!strcmp(pdu_info->participants[1].name, "Klient 2"));
	assert(!strcmp(pdu_info->participants[2].name, "Klient 3"));

	
	return 1;
}


/*
* Test function for pdu_PLEAVE to see if the given input to the
* function will return the expected output.
*/
int test_pdu_PLEAVE()
{
	int unused;
	pdu_PJOIN pj;
	pj.op_code = PLEAVE;
	pj.timestamp = time(NULL);
	pj.identity = "THIS IS MY IDENTITY 111";
	pj.identLen = strlen(pj.identity);

	char *msg = create_PLEAVE(pj, &unused);
	pdu_PLEAVE *pdu_info = parse_PLEAVE(msg);

	assert(pj.op_code == pdu_info->op_code);
	assert(pj.identLen == (int)pdu_info->id_len);
	assert(pj.timestamp == pdu_info->timestamp);
	assert(!strcmp(pj.identity, pdu_info->identity));
	/*
	free(pdu_info->identity);
	free(pdu_info);
	free(msg);

*/
	return 1;
}
/*
* Test function for pdu_NOTREG to see if the given input to the
* function will return the expected output.
*/
int test_pdu_NOTREG(){

	char buff[4];
	unsigned short idnum = 10328;
	idnum = htons(idnum);

	buff[0] = NOTREG;
	buff[1] = PAD;
	memcpy(&buff[2], &idnum, 2);

	pdu_NOTREG test = parse_NOTREG(buff);

	assert(NOTREG == (int)test.op_code);
	assert(10328 == test.identityNum);

	return 1;

}

/*
* Test function for pdu_SLIST to see if the given input to the
* function will return the expected output.
*/
int test_pdu_SLIST(){
	char buffer[] = "40021234((06hej123004321))06hej321\0\0";//test buff to parse
	serverInfo si = parse_SLIST(buffer);
	char *servname1 = "hej123";
	char *servname2 = "hej321";
	buffer[18] = '\0';
	buffer[19] = '\0';
	
	printf("op:%d\n",(int)si.op_code);
	//server 1
	assert(SLIST == (int)si.op_code-48);
	assert(2 == (int)si.nrOfServers-48);
	//assert(1234 == si.serv[0].addr );
	assert(10280 == si.serv[0].portNum);
	assert(0 == (int)si.serv[0].nrClients-48);
	assert(6 == (int)si.serv[0].servNL-48);
	assert( !strcmp(servname1,si.serv[0].servername) );

	//server 2
	//assert(4321 == si.serv[1].addr );
	assert(10537 == si.serv[1].portNum);
	assert(0 == si.serv[1].nrClients-48);
	assert(6 == (int)si.serv[1].servNL-48);
	assert( !strcmp(servname2,si.serv[1].servername) );



	return 1;
}	
/*
* Test function for pdu_MSG to see if the given input to the
* function will return the expected output.
*/
int test_pdu_MSG(){
     
	int unused;
    pdu_MESS m;
    m.op_code = MESS;
    sprintf(m.client_id,"1234");

    m.idLen = strlen(m.client_id);
 	m.crc = '1';
    m.msglen = 6;
    m.timestamp = time(NULL);

    m.msg = "1";
  
    char *buff = create_MESS(m,1, &unused);
    pdu_MESS p = parse_MSG(buff,1);
    for(int i = 0; i < unused; i++){
    	printf("%d ",buff[i]);
    }
    assert(m.op_code == p.op_code); 
    assert(1 == (p.crc-48) );
    assert(m.idLen == p.idLen);
    assert(6 == p.msglen );
    assert(m.timestamp == p.timestamp);
    assert(!strcmp(m.msg,p.msg) );
    assert(!strcmp(m.client_id,p.client_id));
    return 1;
}

//cli 10 0 0 49 0 1 0 0 25 24 -50 0 49 0 0 0 

//test10 0 4 49 0 6 0 0 87 -2 -66 -11 49 0 37 



/*
* Test function for pdu_QUIT to see if the given input to the
* function will return the expected output.
*/
int test_pdu_QUIT(){
	int unused;
	char *buff = create_QUIT(&unused);
	char *buff2 = create_GETLIST(&unused);

	assert(1 == parse_QUIT(buff)  );
	assert(0 == parse_QUIT(buff2) );

	return 1;
}


