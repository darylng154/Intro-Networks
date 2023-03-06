/* Server side - UDP Code				    */
/* By Hugh Smith	4/1/2017	*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/wait.h>
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
	START_STATE, FILENAME, RECV_DATA, WAIT_ON_EOF_ACK, DONE
};

int checkArgs(int argc, char *argv[]);
void handleZombies(int signal);
void processServer(int serverSocketNum, Connection* server);
void talkToClient(Connection* client, uint32_t* serverSeqNum);
void processClient(Connection* client, uint8_t* dataBuffer);
STATE newSocket(Connection* client);
STATE filename(Connection* client, Window* window, uint8_t* dataBuffer, int* toFile);
void parseFilenamePkt(uint8_t* dataBuffer, char* toFilename);
int writeToFile(int toFile, uint8_t* dataBuffer, int dataLen);
STATE recvData(Connection* client, Window* window, int toFile, uint32_t* serverSeqNum);
STATE waitOnEOFACK(Connection* client, uint32_t serverSeqNum);


int main ( int argc, char *argv[]  )
{ 
	int portNumber = 0;
	int serverSocketNum = 0;

	portNumber = checkArgs(argc, argv);
		
	Connection* client = (Connection*) calloc(1, sizeof(Connection));
	serverSocketNum = udpServerSetup(portNumber, client);
	printf("serverSocketNum: %d \n", serverSocketNum);

	sendErr_init(atof(argv[1]), DROP_ON, FLIP_OFF, DEBUG_ON, RSEED_OFF);	// error rate and all flags enabled
	// sendErr_init(atof(argv[1]), DROP_OFF, FLIP_OFF, DEBUG_ON, RSEED_OFF);	// error rate w/ only debug flag
	
	processServer(serverSocketNum, client);

	close(serverSocketNum);

	free(client);
	
	return 0;
}

int checkArgs(int argc, char *argv[])
{
	// Checks args and returns port number
	int portNumber = 0;

	// if (argc < 2 || argc > 3)
	if(argc < 2)
	{
		fprintf(stderr, "Usage %s error_rate [optional port number]\n", argv[0]);
		exit(-1);
	}
	
	if (argc > 2)
	{
		portNumber = atoi(argv[2]);
	}
	else
	{
		portNumber = 0;
	}
	
	if(argc > 3)
		VERBOSE = argv[3][0];

	if(VERBOSE == 'v')
		printf("VERBOSE: %c \n", VERBOSE);

	return portNumber;
}

// SIGCHLD handler - clena up terminated processes
void handleZombies(int signal)
{
	int stat = 0;
	while(waitpid(-1, &stat, WNOHANG) > 0){}
}

void processServer(int serverSocketNum, Connection* client)
{
	// pid_t pid = 0;
	uint8_t dataBuffer[MAXBUF];
	uint32_t recvLen = 0;

	// signal to handle bad fork
	signal(SIGCHLD, handleZombies);
	
	// get new client connection, fork child
	while(1)
	{
		// block waiting for a new client
		if(VERBOSE == 'v')
		{
			printf("recv filename frame \n");
		}

		recvLen = recvfromErr(serverSocketNum, dataBuffer, MAXBUF, 0, (struct sockaddr*) &(client->remote), (socklen_t*) &(client->length));
		
		if(VERBOSE == 'v')
			printBuffer(dataBuffer, recvLen);
		
		if(recvLen > 0)
		{
			recvLen -= HEADERSIZE;
			memcpy(dataBuffer, &(dataBuffer[HEADERSIZE]), recvLen);
			printBuffer(dataBuffer, recvLen);
		}

		if(recvLen != CRC_ERROR)
		{			
			processClient(client, dataBuffer);

			// if((pid = fork()) < 0)
			// {
			// 	perror("#ERROR: fork Failed");
			// 	exit(-1);
			// }

			// if(pid == 0)
			// {
			// 	// child process = new process for each client
			// 	if(VERBOSE == 'v')
			// 		printf("Child fork() - child pid: %d \n", getpid());

			// 	// pass recvLen to child to process FILENAME pkt
			// 	// processClient(client, dataBuffer);
			// 	exit(0);
			// }
		}
	}
}

void talkToClient(Connection* client, uint32_t* serverSeqNum)
{
	uint8_t pduBuffer[MAXBUF];
	int dataLen = 0;
	uint8_t flag = 0;
	uint32_t clientSeqNum;
	
	// pduBuffer[0] = '\0';
	while(1)
	{
		dataLen = recvPDU(client, pduBuffer, BUFSIZE, &clientSeqNum, &flag);

		if(VERBOSE == 'v')
		{
			printf("Received message from client with ");
			printIPInfo(&(client->remote));
			printf(" Len: %d \'%s\'\n", dataLen, pduBuffer);
		}

		sendPDU(client, pduBuffer, dataLen, *serverSeqNum, flag);
		(*serverSeqNum)++;
	}
}

void processClient(Connection* client, uint8_t* dataBuffer)
{
	Window* window = (Window*) calloc(1, sizeof(Window));

	uint32_t serverSeqNum = 0;
	int toFile = -1;
	STATE state = START_STATE;

	while(state != DONE)
	{
		switch(state)
		{
			case START_STATE:	// recved filename pkt & forks
				state = newSocket(client);
				break;

			case FILENAME:
				state = filename(client, window, dataBuffer, &toFile);
				break;

			case RECV_DATA:
				state = recvData(client, window, toFile, &serverSeqNum);
				// talkToClient(client, &serverSeqNum);
				break;

			case WAIT_ON_EOF_ACK:
				state = waitOnEOFACK(client, serverSeqNum);
				break;

			case DONE:
				break;

			default:
				printf("#ERROR: in default state \n");
				break;
		}
	}

	close(toFile);
	cleanup(window);

	// don't free if i dont seperate Connection* client from main
	// close(client->socketNum);
	// free(client);
}

STATE newSocket(Connection* client)
{
	STATE returnValue = DONE;

	// create new client socket for forked child
	// client->socketNum = safeGetUdpSocket();

	// send new socket
	// sendPDU(client, pduBuffer, dataLen, *serverSeqNum, flag);

	setupPollSet();
	addToPollSet(client->socketNum);

	returnValue = FILENAME;

	return returnValue;
}

STATE filename(Connection* client, Window* window, uint8_t* dataBuffer, int* toFile)
{
	uint8_t emptyBuffer[1] = {'\0'};
	char toFilename[MAXFILENAME + 1] = {'\0'};
	STATE returnValue = DONE;

	parseFilenamePkt(dataBuffer, toFilename);

	if((*toFile = open(toFilename, O_CREAT | O_TRUNC | O_WRONLY, 0600)) == -1)
	{
		// sendPDU(client, emptyBuffer, sizeof(emptyBuffer), 0, 0);
		sendPDU(client, emptyBuffer, 0, 0, 0);
		returnValue = DONE;
	}
	else
	{
		initWindow(window, WINDOWSIZE, BUFSIZE);
		sendPDU(client, emptyBuffer, sizeof(emptyBuffer), 0, FNAME_ACK);
		returnValue = RECV_DATA;
	}

	return returnValue;
}

void parseFilenamePkt(uint8_t* dataBuffer, char* toFilename)
{
	int curHeaderLen = 0;

	// parse toFilename & remove toFilenameLen
	curHeaderLen += parseString(dataBuffer, toFilename, curHeaderLen);

	// parse window-size
	memcpy(&(WINDOWSIZE), &(dataBuffer[curHeaderLen]), sizeof(WINDOWSIZE));
	curHeaderLen += sizeof(WINDOWSIZE);

	// parse buffer-size
	memcpy(&(BUFSIZE), &(dataBuffer[curHeaderLen]), sizeof(BUFSIZE));
	// curHeaderLen += sizeof(BUFSIZE);

	if(VERBOSE == 'v')
		printf("to-Filename: %s | WINDOWSIZE: %d | BUFSIZE: %d \n", toFilename, WINDOWSIZE, BUFSIZE);
}

int writeToFile(int toFile, uint8_t* dataBuffer, int dataLen)
{
    int writeLen = 0;

    if((writeLen = write(toFile, dataBuffer, dataLen)) == -1)
    {
        if(VERBOSE == 'v')
            perror("Error on writing to toFile");
            
        exit(-1);
    }

    return writeLen;
}

// add Window to buffer out of seq data
STATE recvData(Connection* client, int toFile, uint32_t* serverSeqNum)
{
	uint8_t dataBuffer[MAXBUF] = {'\0'};
	int readySocket = 0;
	int dataLen = 0;
	int writeLen = 0;
	uint8_t flag = 0;
	uint32_t recvedSeqNum = 0;
	STATE returnValue = DONE;

	readySocket = pollCall(TEN_SEC);

	if(readySocket == client->socketNum)
	{
        dataLen = recvPDU(client, dataBuffer, BUFSIZE, &recvedSeqNum, &flag);

		// if(VERBOSE == 'v')
		// {
		// 	printf("printing dataBuffer\n");
		// 	printBuffer(dataBuffer, dataLen);
		// 	printf("####################################################################\n\n");
		// }
	}
	else if(readySocket == -1)	// nothing is ready to read
	{
		if(VERBOSE == 'v')
			printf("Server 10 sec poll timed out. \n");

		return DONE;
	}

	if(flag == END_OF_FILE)
	{
		if(recvedSeqNum == *serverSeqNum)
		{
			returnValue = WAIT_ON_EOF_ACK;
		}
		else
		{
			memcpy(dataBuffer, serverSeqNum, sizeof(*serverSeqNum));
			sendPDU(client, dataBuffer, sizeof(*serverSeqNum), *serverSeqNum, SREJ);
			returnValue = RECV_DATA;
		}
	}
	else
	{
		if(recvedSeqNum == *serverSeqNum)
		{
			writeLen = writeToFile(toFile, dataBuffer, dataLen);

			memcpy(dataBuffer, serverSeqNum, sizeof(*serverSeqNum));
			sendPDU(client, dataBuffer, sizeof(*serverSeqNum), *serverSeqNum, RR);

			(*serverSeqNum)++;

			returnValue = RECV_DATA;

			printf("readLen: %d | writeLen: %d \n", dataLen, writeLen);
			printf("####################################################################\n\n");
		}
		else
		{
			// buffer data & send SREJs
			
			returnValue = DONE;
		}
	}

	return returnValue;
}

STATE waitOnEOFACK(Connection* client, uint32_t serverSeqNum)
{
	uint8_t dataBuffer[MAXBUF] = {'\0'};
	uint32_t recvedSeqNum = 0;
	uint8_t flag = 0;
	int numRetries = 0;
	int readySocket = -1;
	STATE returnValue = DONE;

	while(numRetries <= MAXRETRIES)
	{
		sendPDU(client, dataBuffer, 0, serverSeqNum, EOF_ACK);
		readySocket = pollCall(TEN_SEC);

		if(readySocket == client->socketNum)
		{
			recvPDU(client, dataBuffer, BUFSIZE, &recvedSeqNum, &flag);

			if(flag == EOF_ACK_ACK)
				return DONE;
		}
		else if(readySocket == -1)	// nothing is ready to read
		{
			if(VERBOSE == 'v')
				printf("Server 10 sec poll timed out. \n");
		}

		if(VERBOSE == 'v')
			printf("readySocket: %d, numRetries: %d \n", readySocket, numRetries);
	}

	if(numRetries >= MAXRETRIES)
	{
		printf("No response from other side for %d seconds: terminating connection\n", numRetries);
	}

	return returnValue;
}