/******************************************************************************
* myClient.c
*
* Writen by Prof. Smith, updated Jan 2023
* Use at your own risk.  
*
*****************************************************************************/

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
#include <stdint.h>
#include <ctype.h>

#include "networks.h"
#include "safeUtil.h"
#include "pollLib.h"

#include "pdu.h"
#include "handleTable.h"

#define ARG_HANDLE_INDEX 1
#define ARG_IP_INDEX 2
#define ARG_PORT_INDEX 3

#define MAXBUF 1400
#define DEBUG_FLAG 1
#define COMMAND_LEN 2
#define COMMAND_INDEX 0
#define NUM_HANDLES_INDEX 1
#define NUM_HANDLES_LEN 1

void sendToServer(int socketNum, uint8_t* sendBuf, int sendLen);
int readFromStdin(uint8_t * buffer);
void checkArgs(int argc, char * argv[]);
void clientControl(int serverSocket, char* handle);
void recvFromServer(int serverSocket, uint8_t* dataBuffer);
void sendInitPkt(int serverSocket, char* handle);
void processStdin(int serverSocket, char* handle);
void processMsgFromServer(int serverSocket, char* handle);
uint8_t processCommand(char* command, int numArgs);
uint8_t processResponse(int serverSocket, uint8_t* dataBuffer, char* handle, char* srcHandle);
int createHeader(uint8_t* pduBuffer, uint8_t flag, char* srcHandle, int numHandles, struct handleTable** destTable, int* destTableLen);
int parseCommand(uint8_t* tokenBuffer, uint8_t* flag, uint8_t* dataBuffer, int* numHandles, struct handleTable** destTable, int* destTableLen);
int createRequest(uint8_t flag, uint8_t* pduBuffer, uint8_t* msgBuffer, int headerLen);
void sendRequest(int serverSocket, uint8_t flag, uint8_t* pduBuffer, int pduLen);

void processBadHandle(uint8_t* dataBuffer);
void processBroadcastMsg(uint8_t* dataBuffer);
void processHandleList(int serverSocket, uint8_t* dataBuffer);
void processMsg(uint8_t* dataBuffer);

int addNumHandles(uint8_t* dataBuffer, struct handleTable* table, int tableLen, int curHeaderLen);

int main(int argc, char * argv[])
{
	int socketNum = 0;         //socket descriptor
	char* handle = NULL;
	
	checkArgs(argc, argv);
	handle = argv[ARG_HANDLE_INDEX];
	// printf("handle: %s \n", handle);

	if(strlen(handle) > MAX_HANDLE_LEN)
	{
		perror("Handle is more than 100 characters. \n");
		exit(-1);
	}

	/* set up the TCP Client socket  */
	socketNum = tcpClientSetup(argv[ARG_IP_INDEX], argv[ARG_PORT_INDEX], DEBUG_FLAG);

	clientControl(socketNum, handle);
	
	close(socketNum);
	
	return 0;
}

void sendToServer(int socketNum, uint8_t* sendBuf, int sendLen)
{
	int sent = 0;            //actual amount of data sent/* get the data and send it   */
	
	printf("read: %s, string len: %d (including null)\n", (char*)sendBuf, sendLen);
	
	sent = sendPDU(socketNum, sendBuf, sendLen);
	if (sent < 0)
	{
		perror("send call");
		exit(-1);
	}

	printf("Amount of data sent is: %d\n\n", sent);
}

int readFromStdin(uint8_t* buffer)
{
	char aChar = 0;
	int inputLen = 0;        
	
	// Important you don't input more characters than you have space 
	buffer[0] = '\0';
	// printf("Enter data: ");
	while (inputLen < (MAXBUF - 1) && aChar != '\n')
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

void checkArgs(int argc, char * argv[])
{
	/* check command line arguments  */
	if (argc != 4)
	{
		printf("usage: %s handle-name host-name port-number \n", argv[0]);
		exit(1);
	}
}

void clientControl(int serverSocket, char* handle)
{
	int readySocket = 0;
	
	setupPollSet();
	addToPollSet(serverSocket);
	addToPollSet(STDIN_FILENO);

	sendInitPkt(serverSocket, handle);

	while(1)
	{
		printf("%s $: ", handle);
		fflush(stdout);

		//poll & block forever
		readySocket = pollCall(-1);

		if(readySocket == STDIN_FILENO)			// socket ready to send
		{
			processStdin(serverSocket, handle);
		}
		else if(readySocket == serverSocket)	// socket ready to recv
		{
			processMsgFromServer(serverSocket, handle);
		}
	}

	printf("cclient exited while loop \n");
}

void recvFromServer(int serverSocket, uint8_t* dataBuffer)
{
	int recved = 0;        			//amount of data to send

	recved = recvPDU(serverSocket, dataBuffer, MAXBUF);
	if(recved > 0)
	{
		printf("Message received on socket %d, length: %d Data: %s", serverSocket, recved, dataBuffer);
		// printf("(note the length is %d because of the null)\n", recved);
		printf("\n\n");
	}
	else if(recved < 0)
	{
		perror("#ERROR: recvFromServer recvPDU < 0");
		exit(-1);
	}
	if(recved == 0)		//checks if socket is closed
	{
		perror("ERROR#: Server closed");
		exit(-1);
	}
}

void sendInitPkt(int serverSocket, char* handle)
{
	int flag = 1;
	uint8_t pduBuffer[MAXBUF];   		// data buffer for PDU 
	int pduLen = 0;
	int readySocket = 0;

	pduLen += addAByte(pduBuffer, flag, FLAG_INDEX);
	pduLen += addHandle(pduBuffer, handle, pduLen);
	
	printf("#### sendInitPkt ####\n");
	printf("check handle arg: %s \n", handle);
	printf("flag: %d \n", flag);
	printf("handleLen: %d \n", pduBuffer[1]);

	printBuffer(pduBuffer, pduLen);
	sendToServer(serverSocket, pduBuffer, pduLen);

	while(readySocket != serverSocket)
	{
		//poll & block forever
		readySocket = pollCall(-1);
	}
}

void processStdin(int serverSocket, char* handle)
{
	uint8_t stdinBuffer[MAXBUF];   		// data buffer for STDIN
	uint8_t tokenBuffer[MAXBUF];   		// data buffer for strtok & 200 msgBuffer (save memory)
	int commandLen = 0;
	uint8_t flag = 0;

	uint8_t pduBuffer[MAXBUF];			// data buffer for PDU (200 Max Text + 900 Max Handles Lenght)
	int pduLen = 0;
	int headerLen = 0;

	int numHandles = 0;
	// char* destHandles[MAX_DEST_HANDLES];

	struct handleTable* destTable = NULL;
	int destTableLen = 2;
    initHandleTables(&destTable, destTableLen);

	int i;
	int numSends = 0;

	// messageLen = readFromStdin(stdinBuffer);
	readFromStdin(stdinBuffer);
	// flag = processCommand(stdinBuffer);

	// if(flag == 5 || flag == 6)
		// commandLen = parseCommand(tokenBuffer, stdinBuffer, &numHandles, destHandles);
		commandLen = parseCommand(tokenBuffer, &flag, stdinBuffer, &numHandles, &destTable, &destTableLen);		
	// else
	// 	commandLen = 3;

	if(commandLen == -1)
	{
		// printf("commandLen: %d \n", commandLen);
		return;
	}

	headerLen = createHeader(pduBuffer, flag, handle, numHandles, &destTable, &destTableLen);
	
	// printf("%s \n", (char*)stdinBuffer);

	// clear the commands and leave only the message
	memcpy(stdinBuffer, &stdinBuffer[commandLen], MAXBUF);

	printf("#### AFTER MEMCPY STDIN BUFFER ####\n");
	printf("headerLen: %d \n", headerLen);
	printf("%s \n", (char*)stdinBuffer);

	numSends = getNumSends(stdinBuffer);
	if(flag == 1 || flag == 10 || flag == 8)
		numSends = 1;

	for(i = 0; i < numSends; i++)
	{
		//split the message
		next200Msg(tokenBuffer, stdinBuffer);

		pduLen = createRequest(flag, pduBuffer, tokenBuffer, headerLen) + headerLen;

		if(pduLen != -1)
			sendToServer(serverSocket, pduBuffer, pduLen);
	}
}

void processMsgFromServer(int serverSocket, char* handle)
{
	uint8_t dataBuffer[MAXBUF] = {'\0'};
	uint8_t flag = 0;
	char srcHandle[MAX_HANDLE_LEN] = {'\0'};

	recvFromServer(serverSocket, dataBuffer);

	printBuffer(dataBuffer, MAXBUF);

	flag = processResponse(serverSocket, dataBuffer, handle, srcHandle);

	printf("#### processMsgFromServer ####\n");
	printf("flag: %d \n", flag);
}

uint8_t processCommand(char* command, int numArgs)
{
	uint8_t flag = 0;

	if(isalpha(command[1]) != 0)
	{
		command[1] = tolower(command[1]);
	}

	if(command[1] == 'e')
	{
		flag = 8;
	}
	else if(command[1] == 'l')
	{
		flag = 10;
	}
	else if(command[1] == 'm')
	{
		flag = 5;
	}
	else if(command[1] == 'c')
	{
		flag = 6;
	}
	else if(command[1] == 'b')
	{
		flag = 4;
	}
	else
	{
		perror("Invalid Command.");
	}

	printf("command: %c, flag: %d \n", command[1], flag);

	return flag;
}

int createHeader(uint8_t* pduBuffer, uint8_t flag, char* srcHandle, int numHandles, struct handleTable** destTable, int* destTableLen)
{
	// uint8_t pduBuffer[MAXBUF];	// data buffer for PDU 
	int pduLen = 0;

	printf("#### createHeader ####\n");

	switch(flag)
	{
		case 4:
			pduLen += addAByte(pduBuffer, flag, FLAG_INDEX);
			pduLen += addHandle(pduBuffer, srcHandle, pduLen);
			break;

		// case 5:	
		// 	pduLen += addAByte(pduBuffer, flag, FLAG_INDEX);
		// 	pduLen += addHandle(pduBuffer, srcHandle, pduLen);		// src
		// 	pduLen += addAByte(pduBuffer, numHandles, pduLen);
		// 	pduLen += addHandle(pduBuffer, getHandleByIndex(*destTable, *destTableLen, 0), pduLen);	// dest
		// 	// printf("destHandle[0]: %s \n", destHandles[0]);
		// 	break;

		case 5: case 6:
			pduLen += addAByte(pduBuffer, flag, FLAG_INDEX);
			pduLen += addHandle(pduBuffer, srcHandle, pduLen);		// src
			pduLen += addAByte(pduBuffer, numHandles, pduLen);
			printf("pduLen before addNumHandles: %d \n ", pduLen);

			pduLen = addNumHandles(pduBuffer, *destTable, *destTableLen, pduLen);
			break;

		case 8:
			pduLen += addAByte(pduBuffer, flag, FLAG_INDEX);
			break;

		case 10:
			pduLen += addAByte(pduBuffer, flag, FLAG_INDEX);
			break;

		default:
			perror("#ERROR: createAndSendCommand defaulted");
			// exit(-1);
			break;
	}

	printf("headerLen: %i \n", pduLen);
	// Note: dataBuffer doesn't includes pduLen (2 Bytes) in chat header
	// printBuffer(pduBuffer, pduLen);

	return pduLen;
}

uint8_t processResponse(int serverSocket, uint8_t* dataBuffer, char* handle, char* srcHandle)
{
	uint8_t flag = 0;
	char aHandle[MAX_HANDLE_LEN] = {'\0'};
	int curHeaderLen = 0;

	flag = parseAByte(dataBuffer, FLAG_INDEX);
	// clearPDUBuffer(msgBuffer);

	printf("#### processFlag ####\n");
	printf("flag: %d \n", flag);

	switch(flag)
	{
		case 2:	// initial pdu good handle
			// process and move on
			break;

		case 3:	// initial pdu handle already exists
			printf("Handle already in use: %s \n", handle);
			exit(-1);
			break;

		case 4:
			processBroadcastMsg(dataBuffer);
			break;

		case 5: case 6:
			processMsg(dataBuffer);
			break;

		case 7:
			processBadHandle(dataBuffer);
			break;

		case 9:
			exit(-1);
			break;

		case 11:
			processHandleList(serverSocket, dataBuffer);
			break;

		case 12:
			curHeaderLen = 1;
			parseHandle(dataBuffer, aHandle, curHeaderLen);
			printf("\t%s \n", aHandle);
			break;

		case 13:
			// moves on
			break;

		default:
			perror("#ERROR: processFlag defaulted");
			// exit(-1);
			break;
	}

	return flag;
}

int getNumArgs(uint8_t* tokenBuffer)
{
	uint8_t* copyBuffer[MAXBUF];
	int i;
	char* token;

	memcpy(copyBuffer, tokenBuffer, MAXBUF);
	token = strtok((char*)copyBuffer, " ");	// parses each string seperated by space
	while(token != NULL)
	{
		i++;
		token = strtok(NULL, " ");			// skip command
	};

	return i;
}

int parseCommand(uint8_t* tokenBuffer, uint8_t* flag, uint8_t* dataBuffer, int* numHandles, struct handleTable** destTable, int* destTableLen)
{
	char command[COMMAND_LEN + 1];
	int commandLen = 0;
	char* token = NULL;
	int i = 0;
	int numArgs = 0;
	
	// numArgs = getNumArgs(tokenBuffer);

	memcpy(tokenBuffer, dataBuffer, MAXBUF);
	token = strtok((char*)tokenBuffer, " ");	// parses each string seperated by space

	if(strlen(token) > COMMAND_LEN)
		return -1;
	else
		memcpy(command, token, COMMAND_LEN);
	commandLen += (strlen(token) + 1);	// +1 = space char
	token = strtok(NULL, " ");			// skip command

	*flag = processCommand(command, numArgs);
	printf("numArgs: %d \n", numArgs);

	if(*flag == 5 || *flag == 6)
	{

		if(strlen(token) > NUM_HANDLES_LEN)
			return -1;
		else
			*numHandles = atoi(token);		// get number of handles

		commandLen += (strlen(token) + 1);	// +1 = space char
		token = strtok(NULL, " ");			// skip command

		printf("numHandles: %d \n", *numHandles);

		for(i = 0; i < *numHandles; i++)
		{
			printf("%s \n", token);

			if(strlen(token) > MAX_HANDLE_LEN)
			{
				perror("Handle is more than 100 characters. \n");
				return -1;
			}

			*destTableLen = addHandleToTable(destTable, *destTableLen, -1, token);
			commandLen += (strlen(token) + 1);

			token = strtok(NULL, " ");
		}

		printHandleTables(*destTable, *destTableLen);
	}

	printf("commandLen: %d \n", commandLen);

	return commandLen;
}

int createRequest(uint8_t flag, uint8_t* pduBuffer, uint8_t* msgBuffer, int headerLen)
{
	int pduLen = 0;

	switch(flag)
	{
		case 4: case 5:	case 6: case 10:
			pduLen += addMessage(pduBuffer, msgBuffer, headerLen);

			printBuffer(pduBuffer, pduLen + headerLen);
			break;

		case 8:
			break;

		default:
			perror("#ERROR: createRequest defaulted");
			return -1;
			break;
	}

	printf("#### createRequest ####\n");
	printf("pduLen: %d \n", pduLen);
	return pduLen;
}

void processBadHandle(uint8_t* dataBuffer)
{
	uint8_t msgBuffer[MAX_MSG] = {'\0'};
	char badHandle[MAX_HANDLE_LEN] = {'\0'};

	parseHandle(dataBuffer, badHandle, 1);

	strcat((char*)msgBuffer, "Client with handle ");
	strcat((char*)msgBuffer, badHandle);
	strcat((char*)msgBuffer, " does not exist.");
	printf("%s \n", msgBuffer);
}

void processBroadcastMsg(uint8_t* dataBuffer)
{
	int curHeaderLen = 1;
	int srcHandleLen = 0;
	char srcHandle[MAX_HANDLE_LEN];
	uint8_t msgBuffer[MAX_MSG] = {'\0'};

	srcHandleLen = parseHandle(dataBuffer, srcHandle, 1);
	printf("curHeaderLen: %d \n", curHeaderLen);
	curHeaderLen += srcHandleLen;
	printf("curHeaderLen: %d \n", curHeaderLen);

	memcpy(msgBuffer, &dataBuffer[curHeaderLen], MAX_MSG);

	printf("#### processBroadcastMsg ####\n");
	printBuffer(dataBuffer, MAX_MSG);
	printf("srcHandleLen: %d , srcHandle: %s \n", srcHandleLen, srcHandle);
	printf("curHeaderLen: %d \n", curHeaderLen);
	printf("strlen of msg: %d \n", (int)strlen((char*)&dataBuffer[curHeaderLen]));
	printBuffer(msgBuffer, MAX_MSG);

	printf("%s: %s \n", srcHandle, msgBuffer);
}

void processHandleList(int serverSocket, uint8_t* dataBuffer)
{
	int numHandles = 0;
	int curHeaderLen = 0;

	curHeaderLen = 1;
	numHandles = parseNumHandlesInTable(dataBuffer, curHeaderLen);
	printf("Number of clients: %d \n", numHandles);
}

void processMsg(uint8_t* dataBuffer)
{
	int curHeaderLen = 1;
	int srcHandleLen = 0;
	char srcHandle[MAX_HANDLE_LEN];
	uint8_t msgBuffer[MAX_MSG] = {'\0'};
	char destBuffer[MAX_HANDLE_LEN];

	int numHandles = 0;
	int i;

	srcHandleLen = parseHandle(dataBuffer, srcHandle, 1);
	printf("curHeaderLen: %d \n", curHeaderLen);
	curHeaderLen += srcHandleLen;
	printf("curHeaderLen: %d \n", curHeaderLen);

	numHandles = parseAByte(dataBuffer, curHeaderLen);
	curHeaderLen++;
	printf("curHeaderLen: %d \n", curHeaderLen);

	for(i = 0; i < numHandles; i++)
	{
		curHeaderLen += parseHandle(dataBuffer, destBuffer, curHeaderLen);
	}
	printf("curHeaderLen: %d \n", curHeaderLen);

	memcpy(msgBuffer, &dataBuffer[curHeaderLen], MAX_MSG+1);

	printf("#### processMsg ####\n");
	printBuffer(dataBuffer, MAX_MSG+1);
	printf("srcHandleLen: %d , srcHandle: %s \n", srcHandleLen, srcHandle);
	printf("curHeaderLen: %d \n", curHeaderLen);
	printf("strlen of msg: %d \n", (int)strlen((char*)&dataBuffer[curHeaderLen]));
	printBuffer(msgBuffer, MAX_MSG+1);

	printf("%s: %s \n", srcHandle, msgBuffer);
}

int addNumHandles(uint8_t* dataBuffer, struct handleTable* table, int tableLen, int curHeaderLen)
{
    int i;
	int pduLen = curHeaderLen;

	printf("#### addNumHandles ####\n");
	printHandleTables(table, tableLen);
    for(i = 0; i < tableLen; i++)
    {
        if(!isEmpty(table[i]))
        {
			pduLen += addHandle(dataBuffer, getHandleByIndex(table, tableLen, i), pduLen);	// dest
			printf("pduLen: %d, in index: %d \n", pduLen, i);
        }
		printf("pduLen: %d, out index: %d \n", pduLen, i);
    }

    return pduLen;
}