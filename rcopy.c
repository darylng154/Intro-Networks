// Client side - UDP Code				    
// By Hugh Smith	4/1/2017		

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <fcntl.h>

#include "gethostbyname.h"
#include "networks.h"
#include "safeUtil.h"
#include "cpe464.h"
#include "pollLib.h"

#include "pdu.h"
#include "window.h"

int16_t BUFSIZE = 0;
uint32_t WINDOWSIZE = 0;
char VERBOSE = '\0';

typedef enum State STATE;

enum State
{
	START_STATE, FILENAME, SEND_DATA, WINDOW_OPEN, WINDOW_CLOSED, WAIT_ON_EOF, DONE
};

int checkArgs(int argc, char * argv[]);
void talkToServer(Connection* server, uint32_t* clientSeqNum);
int readFromStdin(char * buffer);
void processFile(char** argv);
int retryPoll(Connection* server, int* numRetries, int pollTime);
STATE startState(Connection* server, char** argv,  int* fromFile);
STATE filename(Connection* server, char** argv);
int createFilenamePkt(uint8_t* dataBuffer, char** argv);
STATE checkWindow(Window* window);
STATE sendData(Connection* server, Window* window, int* fromFile, uint32_t* clientSeqNum);
void processRRorSREJ(Connection* server, Window* window, uint32_t serverSeqNum, uint8_t flag);
STATE windowClosed(Connection* server, Window* window);


int main (int argc, char *argv[])
{	
	checkArgs(argc, argv);
	
	sendErr_init(atof(argv[5]), DROP_ON, FLIP_OFF, DEBUG_ON, RSEED_OFF);	// error rate and all flags enabled
	// sendErr_init(atof(argv[5]), DROP_OFF, FLIP_OFF, DEBUG_ON, RSEED_OFF);	// error rate w/ only debug flag

	processFile(argv);
	
	return 0;
}

int checkArgs(int argc, char * argv[])
{
	int portNumber = 0;

	/* check command line arguments  */
	if (argc < 8)
	// if (argc != 8)
	{
		printf("usage: %s from-filename to-filename window-size buffer-size error-percent remote-machine remote-port \n", argv[0]);
		exit(-1);
	}

	if(strlen(argv[1]) > MAXFILENAME)
	{
		printf("from-filename is too long. needs to be less than 100 chars but is %d \n", (int)strlen(argv[1]));
		exit(-1);
	}

	if(strlen(argv[2]) > MAXFILENAME)
	{
		printf("to-filename is too long. needs to be less than 100 chars but is %d \n", (int)strlen(argv[2]));
		exit(-1);
	}

	if(atoi(argv[3]) > 1073741824)
	{
		printf("window size needs to be less than 2^30(1,073,741,824) bytes but is %d \n", atoi(argv[3]));
		exit(-1);
	}

	if(atoi(argv[4]) < 1 || atoi(argv[4]) > 1400)
	{
		printf("pduBuffer size needs to be between 1 to 1400 bytes but is %d \n", atoi(argv[4]));
		exit(-1);
	}

	if(atoi(argv[5]) < 0 || atoi(argv[5]) >= 1)
	{
		printf("error rate needs to be between 0 and less than 1 but is %d \n", atoi(argv[5]));
		exit(-1);
	}

	WINDOWSIZE = atoi(argv[3]);
	BUFSIZE = atoi(argv[4]);
	portNumber = atoi(argv[7]);

	if(argc > 8)
		VERBOSE = argv[8][0];

	if(VERBOSE == 'v')
		printf("VERBOSE: %c | from-Filename: %s | to-Filename: %s | WINDOWSIZE: %d | BUFSIZE: %d \n", VERBOSE, argv[1], argv[2], WINDOWSIZE, BUFSIZE);
		
	return portNumber;
}

void talkToServer(Connection* server, uint32_t* clientSeqNum)
{
	// int serverAddrLen = sizeof(struct sockaddr_in6);
	char * ipString = NULL;
	int pduLen = 0; 
	uint8_t pduBuffer[MAXBUF];
	uint32_t serverSeqNum;

	uint8_t flag = 0;

	if(VERBOSE == 'v')
		printf("sizeof(pduBuffer): %d\n", (int)sizeof(pduBuffer));
	
	pduBuffer[0] = '\0';
	while(1)
	{
		pduLen = readFromStdin((char*)pduBuffer);

		printf("Sending: %s with len: %d\n", pduBuffer, pduLen);
	
		sendPDU(server, pduBuffer, pduLen, *clientSeqNum, flag);
		(*clientSeqNum)++;
		
		recvPDU(server, pduBuffer, MAXBUF, &serverSeqNum, &flag);
		
		// print out bytes received
		ipString = ipAddressToString(&(server->remote));
		printf("Server with ip: %s and port %d said it received %s\n", ipString, ntohs((&(server->remote))->sin6_port), pduBuffer);
	}
}

int readFromStdin(char * buffer)
{
	char aChar = 0;
	int inputLen = 0;        
	
	// Important you don't input more characters than you have space 
	buffer[0] = '\0';
	printf("Enter data: ");
	while (inputLen < (BUFSIZE - 1) && aChar != '\n')
	{
		aChar = getchar();
		if (aChar != '\n')
		{
			buffer[inputLen] = aChar;
			inputLen++;
		}
	}
	
	// Null terminate the string
	buffer[inputLen] = '\0';
	inputLen++;
	
	return inputLen;
}

void processFile(char** argv)
{
	Connection* server = (Connection*) calloc(1, sizeof(Connection));
	server->length = sizeof(struct sockaddr_in6);

	Window* window = (Window*) calloc(1, sizeof(Window));
	initWindow(window, WINDOWSIZE, BUFSIZE);

	uint32_t clientSeqNum = 0;
	// uint32_t serverSeqNum = 0;
	int fromFile = -1;
	STATE state = START_STATE;

	while(state != DONE)
	{
		switch(state)
		{
			case START_STATE:
				state = startState(server, argv, &fromFile);
				break;

			case FILENAME:	// send & recv filename
				state = filename(server, argv);
				break;

			case SEND_DATA:
				state = checkWindow(window);
				// talkToServer(server, &clientSeqNum);
				break;

			case WINDOW_OPEN:
				state = sendData(server, window, &fromFile, &clientSeqNum);
				break;

			case WINDOW_CLOSED:
				state = windowClosed(server, window);
				break;

			case DONE:
				break;

			default:
				printf("#ERROR: in default state \n");
				break;
		}
	}

	cleanup(window);
	close(server->socketNum);
	free(server);
}

int retryPoll(Connection* server, int* numRetries, int pollTime)
{
	int readySocket = 0;

	if(*numRetries >= MAXRETRIES)
	{
		printf("No response from other side for %d seconds: terminating connection\n", *numRetries);
		readySocket = -1;
	}

	readySocket = pollCall(pollTime);

	if(readySocket == server->socketNum)
	{
        (*numRetries) = 0;
	}
	else if(readySocket == -1)	// nothing is ready to read
	{
		(*numRetries)++;
	}

	return readySocket;
}

STATE startState(Connection* server, char** argv,  int* fromFile)
{
	STATE returnValue = DONE;

	// if we are connected to server before, close it before reconnecting
	if(server->socketNum > 0)
	{
		close(server->socketNum);
	}

	if((server->socketNum = setupUdpClientToServer(&(server->remote), argv[6], atoi(argv[7]))) < 0)
	{
		returnValue = DONE;
	}
	else
	{
		printf("trying to open file \n");
		if((*fromFile = open(argv[1], O_RDONLY)) == -1)
		{
			printf("Error: file %s not found. \n", argv[1]);
			returnValue = DONE;
		}
		else
		{
			setupPollSet();
			addToPollSet(server->socketNum);

			returnValue = FILENAME;
		}
	}

	return returnValue;
}

STATE filename(Connection* server, char** argv)
{
	uint8_t pduBuffer[MAXBUF];
	uint8_t dataBuffer[MAXBUF];
	int pduLen = 0;
	int readySocket = -1;
	int numRetries = 0;
	uint32_t serverSeqNum = 0;
	uint8_t flag = 0;
	STATE returnValue = DONE;

	pduLen = createFilenamePkt(dataBuffer, argv);

	while(numRetries <= MAXRETRIES && readySocket == -1)
	{
		sendPDU(server, dataBuffer, pduLen, 0, FNAME);
		readySocket = retryPoll(server, &numRetries, ONE_SEC);

		if(VERBOSE == 'v')
			printf("readySocket: %d, numRetries: %d \n", readySocket, numRetries);
	}

	if(readySocket == server->socketNum)
	{
		pduLen = recvPDU(server, pduBuffer, BUFSIZE, &serverSeqNum, &flag);
		
		if(VERBOSE == 'v')
			printf("recved Filename Response flag: %d \n", flag);

		if(flag == FNAME_ACK)
			returnValue = SEND_DATA;
		else
		{
			printf("Error on opening output file on server: %s \n", argv[2]);
			returnValue = DONE;
		}
	}

	return returnValue;
}

int createFilenamePkt(uint8_t* dataBuffer, char** argv)
{
	int curHeaderLen = 0;

	// add toFilenameLen & toFilename
	curHeaderLen += addString(dataBuffer, argv[2], curHeaderLen);

	// add window-size
	memcpy(&(dataBuffer[curHeaderLen]), &(WINDOWSIZE), sizeof(WINDOWSIZE));
	curHeaderLen += sizeof(WINDOWSIZE);

	// add buffer-size
	memcpy(&(dataBuffer[curHeaderLen]), &(BUFSIZE), sizeof(BUFSIZE));
	curHeaderLen += sizeof(BUFSIZE);

	return curHeaderLen;
}

STATE checkWindow(Window* window)
{
	STATE returnValue = DONE;

	if(VERBOSE == 'v')
	{
		printf("####################################################################\n");
		printWindow(window);
	}

	if(getCurrent(window) == getUpper(window))
		returnValue = WINDOW_CLOSED;
	else
		returnValue = WINDOW_OPEN;

	return returnValue;
}

STATE sendData(Connection* server, Window* window, int* fromFile, uint32_t* clientSeqNum)
{
	uint8_t dataBuffer[MAXBUF];
	uint8_t pduBuffer[MAXBUF];
	int readLen = 0;
	int readySocket = 0;
	// int dataLen = 0;
	uint32_t serverSeqNum = 0;
	uint8_t flag = 0;
	STATE returnValue = DONE;

	readLen = read(*fromFile, dataBuffer, BUFSIZE);

	switch(readLen)
	{
		case -1:
			perror("sendData could not read from file");
			returnValue = DONE;
			break;
		
		case 0:	// read 0 bytes == EOF
			returnValue = DONE;
			// returnValue = WAIT_ON_EOF;
			break;

		default:
			addToWindow(window, dataBuffer, BUFSIZE, *clientSeqNum);

			setCurrent(window, getCurrent(window) + 1);

			// remove later, only set current when RR comes
			// setLower(window, getLower(window) + 1);
			
			sendPDU(server, dataBuffer, readLen, *clientSeqNum, DATA);
			(*clientSeqNum)++;

			readySocket = pollCall(0);

			if(readySocket == server->socketNum)
			{
				// process RR & SREJ
				recvPDU(server, pduBuffer, (BUFSIZE + HEADERSIZE), &serverSeqNum, &flag);
				processRRorSREJ(server, window, serverSeqNum, flag);
			}
			
			returnValue = SEND_DATA;
			break;
	}

	return returnValue;
}

void processRRorSREJ(Connection* server, Window* window, uint32_t serverSeqNum, uint8_t flag)
{
	if(flag == RR)
	{
		setLower(window, serverSeqNum);
	}
	else if(flag == SREJ)
	{
		// send SREJ data
		if(VERBOSE == 'v')
			printf("SREJ flag \n");
	}
}

STATE windowClosed(Connection* server, Window* window)
{
	// might need a while not EOF loop: professor's piazza
	uint8_t pduBuffer[MAXBUF];
	uint8_t dataBuffer[MAXBUF];
	int readySocket = -1;
	int numRetries = 0;
	uint32_t serverSeqNum = 0;
	uint8_t flag = 0;
	STATE returnValue = DONE;

	while(numRetries <= MAXRETRIES + 1 && readySocket == -1)
	{
		readySocket = retryPoll(server, &numRetries, ONE_SEC);
		
		if(readySocket == server->socketNum)
		{
			recvPDU(server, pduBuffer, (BUFSIZE + HEADERSIZE), &serverSeqNum, &flag);
			processRRorSREJ(server, window, serverSeqNum, flag);
		}
		else if(readySocket == -1)
		{
			copyDataAtIndex(dataBuffer, window, (getLower(window) % WINDOWSIZE));
			sendPDU(server, dataBuffer, BUFSIZE, getLower(window), DATA);
		}

		if(VERBOSE == 'v')
			printf("readySocket: %d, numRetries: %d \n", readySocket, numRetries);
	}

	returnValue = SEND_DATA;

	return returnValue;
}