/******************************************************************************
* myServer.c
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

#include "networks.h"
#include "safeUtil.h"
#include "pollLib.h"

#include "pdu.h"
#include "handleTable.h"

#define MAXBUF 1400
#define DEBUG_FLAG 1

int recvFromClient(int clientSocket, uint8_t* dataBuffer, struct handleTable* table, int tableLen);
int checkArgs(int argc, char *argv[]);
void serverControl(int mainServerSocket);
void addNewClient(int serverSocket, int debugFlag);
void processClient(int clientSocket, struct handleTable** table, int* tableLen);
void sendToClient(int clientSocket, uint8_t* dataBuffer, int sendLen);
uint8_t processFlag(int clientSocket, uint8_t* dataBuffer, struct handleTable** table, int* tableLen, char* srcHandle);
uint8_t processInitSetup(int clientSocket, uint8_t* dataBuffer, struct handleTable** table, int* tableLen);

int createResponse(int clientSocket, uint8_t* pduBuffer, uint8_t* dataBuffer, uint8_t* flag, int dataLen, 
	struct handleTable** table, int* tableLen, char* srcHandle, struct handleTable** destTable, int* destTableLen);

void sendResponse(int clientSocket, uint8_t* pduBuffer, uint8_t flag, int pduLen, struct handleTable** table, 
int tableLen, char* srcHandle, struct handleTable* destTable, int destTableLen);

void processBroadcastMsg(uint8_t* msgBuffer, uint8_t* dataBuffer, char* srcHandle);
void sendAllHandlesFromTable(int clientSocket, struct handleTable** table, int tableLen);
void sendHandleListACK(int clientSocket);

int processClientToSend(int clientSocket, uint8_t* pduBuffer, uint8_t* dataBuffer, uint8_t* flag, int pduLen, 
	struct handleTable** table, int* tableLen, char* destHandle);

int parseNumHandles(uint8_t* dataBuffer, struct handleTable** table, int* tableLen, struct handleTable** destTable, int* destTableLen, int curHeaderLen, int numHandles);
void processAndSendClientToSend(int clientSocket, uint8_t* pduBuffer, uint8_t* dataBuffer, uint8_t* flag, int pduLen, 
	struct handleTable** table, int* tableLen, struct handleTable** destTable, int* destTableLen);

void sendToClientNumHandles(int clientSocket, uint8_t* pduBuffer, int pduLen, struct handleTable* destTable, int destTableLen);


int main(int argc, char *argv[])
{
	int mainServerSocket = 0;   //socket descriptor for the server socket
	int clientSocket = 0;   	//socket descriptor for the client socket
	int portNumber = 0;
	
	portNumber = checkArgs(argc, argv);
	
	//create the server socket
	mainServerSocket = tcpServerSetup(portNumber);

	serverControl(mainServerSocket);
	
	/* close the sockets */
	close(clientSocket);
	close(mainServerSocket);

	return 0;
}

int recvFromClient(int clientSocket, uint8_t* dataBuffer, struct handleTable* table, int tableLen)
{
	// uint8_t dataBuffer[MAXBUF];
	int messageLen = 0;
	int findSocket = 0;
	
	//now get the data from the client_socket
	if ((messageLen = recvPDU(clientSocket, dataBuffer, MAXBUF)) < 0)
	{
		perror("#ERROR: recvFromClient recvPDU");
		exit(-1);
	}

	if (messageLen > 0)
	{
		printf("Message received on socket %d, length: %d Data: %s \n", clientSocket, messageLen, dataBuffer);
		// printf("(note the length is %d because of the null)\n", messageLen);
	}
	if(messageLen == 0 || strcmp((char*)dataBuffer, "exit") == 0)		//checks if socket is closed first
	{
		removeFromPollSet(clientSocket);
		close(clientSocket);

		if((findSocket = findSocketNumIndex(table, tableLen, clientSocket)) != -1)
		{
			deleteHandleAtIndex(table, tableLen, findSocket);
			printf("#### recvFromClient ####\n");
			printHandleTables(table, tableLen);
		}
	}
	
	return messageLen;
}

int checkArgs(int argc, char *argv[])
{
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 2)
	{
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(-1);
	}
	
	if (argc == 2)
	{
		portNumber = atoi(argv[1]);
	}
	
	return portNumber;
}

void serverControl(int mainServerSocket)
{
	int readySocket = 0;
	setupPollSet();
	addToPollSet(mainServerSocket);

	int tableLen = 2;
    struct handleTable* table = NULL;
    initHandleTables(&table, tableLen);

	while(1)
	{
		//poll & block forever
		readySocket = pollCall(-1);	

		if(readySocket == mainServerSocket)
		{
			addNewClient(readySocket, DEBUG_FLAG);
		}
		else if(readySocket != -1)	//pollCall() == -1 => nothing is ready to read
		{
			processClient(readySocket, &table, &tableLen);
		}
	}

	free(table);
}

void addNewClient(int serverSocket, int debugFlag)
{
	int clientSocket = 0;
	clientSocket = tcpAccept(serverSocket, debugFlag);
	addToPollSet(clientSocket);
}

void processClient(int clientSocket, struct handleTable** table, int* tableLen)
{
	uint8_t dataBuffer[MAXBUF];
	uint8_t pduBuffer[MAXBUF];   		// data buffer for PDU 
	int pduLen = 0;
	uint8_t flag = 0;
	char srcHandle[MAX_HANDLE_LEN] = {'\0'};

	struct handleTable* destTable = NULL;
	int destTableLen = 2;
    initHandleTables(&destTable, destTableLen);

	pduLen = recvFromClient(clientSocket, dataBuffer, *table, *tableLen);

	printf("#### processClient recvFromClient ####\n");
	// Note: dataBuffer doesn't includes pduLen (2 Bytes) in chat header
	printBuffer(dataBuffer, pduLen);

	pduLen = createResponse(clientSocket, pduBuffer, dataBuffer, &flag, pduLen, table, tableLen, srcHandle, &destTable, &destTableLen);
	if(pduLen != -1 && flag != 7 && flag != 6)
		sendResponse(clientSocket, pduBuffer, flag, pduLen, table, *tableLen, srcHandle, destTable, destTableLen);
}

void sendToClient(int clientSocket, uint8_t* dataBuffer, int sendLen)
{
	int sent = 0;

	sent = sendPDU(clientSocket, dataBuffer, sendLen/*, flag*/);	//echo back to client
	if(sent < 0)
	{
		perror("#ERROR: sendToServer sendPDU");
		exit(-1);
	}
	printf("sendToClient: %s string len: %d (including null)\n", dataBuffer, sendLen);
	printf("Amount of data sent is: %d\n", sent);
}

uint8_t processInitSetup(int clientSocket, uint8_t* dataBuffer, struct handleTable** table, int* tableLen)
{
	uint8_t flag = 0;
	char handle[MAX_HANDLE_LEN] = {'\0'};

	parseHandle(dataBuffer, handle, HANDLE_LENGTH_INDEX);

	if(findHandleIndex(*table, *tableLen, handle) == -1)
	{
		*tableLen = addHandleToTable(table, *tableLen, clientSocket, handle);
		flag = 2;
	}
	else
		flag = 3;

	printf("#### processInitClientPkt ####\n");
	printHandleTables(*table, *tableLen);
	printf("flag: %d \n", flag);
	printf("handleLen: %d, handle: %s \n", (int)strlen(handle), handle);

	return flag;
}

int createResponse(int clientSocket, uint8_t* pduBuffer, uint8_t* dataBuffer, uint8_t* flag, int dataLen, 
	struct handleTable** table, int* tableLen, char* srcHandle, struct handleTable** destTable, int* destTableLen)
{
	int pduLen = 0;
	int curHeaderLen = 1;
	uint32_t numHandles = 0;
	// char destHandle[MAX_HANDLE_LEN] = {'\0'};

	*flag = parseAByte(dataBuffer, FLAG_INDEX);

	printf("#### createResponse ####\n");
	printf("flag: %d \n", *flag);

	switch(*flag)
	{
		case 1:	// recved client inital pkt
			*flag = processInitSetup(clientSocket, dataBuffer, table, tableLen);
			pduLen += addAByte(pduBuffer, *flag, FLAG_INDEX);
			break;

		case 4:	
			parseHandle(dataBuffer, srcHandle, 1);
			pduLen = dataLen;
			memcpy(pduBuffer, dataBuffer, pduLen);
			break;

		// case 5:
		// 	curHeaderLen += parseHandle(dataBuffer, srcHandle, curHeaderLen);
		// 	numHandles = parseAByte(dataBuffer, curHeaderLen);	//numHandles
		// 	curHeaderLen++;
		// 	curHeaderLen += parseHandle(dataBuffer, (*destTable)[0].handle, curHeaderLen);
			
		// 	pduLen = dataLen;
		// 	pduLen = processClientToSend(clientSocket, pduBuffer, dataBuffer, flag, pduLen, table, tableLen, (*destTable)[0].handle);
			
		// 	printf("curHeaderLen: %d \n", curHeaderLen);
		// 	printf("destHandle: %s \n", (*destTable)[0].handle);
		// 	break;

		case 5: case 6:
			curHeaderLen += parseHandle(dataBuffer, srcHandle, curHeaderLen);
			numHandles = parseAByte(dataBuffer, curHeaderLen);	//numHandles
			curHeaderLen++;

			// curHeaderLen += parseHandle(dataBuffer, (*destTable)[0].handle, curHeaderLen);
			curHeaderLen = parseNumHandles(dataBuffer, table, tableLen, destTable, destTableLen, curHeaderLen, numHandles);

			printf("curHeaderLen: %d \n", curHeaderLen);
			printHandleTables(*destTable, *destTableLen);

			pduLen = dataLen;
			processAndSendClientToSend(clientSocket, pduBuffer, dataBuffer, flag, pduLen, table, tableLen, destTable, destTableLen); 
			break;

		case 8:
			*flag = 9;
			pduLen += addAByte(pduBuffer, *flag, FLAG_INDEX);
			break;

		case 10:
			*flag = 11;
			pduLen += addAByte(pduBuffer, *flag, FLAG_INDEX);

			numHandles = numEntries(*table, *tableLen);
			pduLen += addNumHandlesInTable(pduBuffer, (uint32_t)numHandles, pduLen);

			printf("numHandles: %d (%u)\n", numHandles, (uint32_t)numHandles);
			break;

		default:
			perror("#ERROR: createResponse defaulted");
			return -1;
			break;
	}

	printf("pduLen: %d \n", pduLen);
	// Note: dataBuffer doesn't includes pduLen (2 Bytes) in chat header
	printBuffer(pduBuffer, pduLen);

	return pduLen;
}

void sendResponse(int clientSocket, uint8_t* pduBuffer, uint8_t flag, int pduLen,
	struct handleTable** table, int tableLen, char* srcHandle, struct handleTable* destTable, int destTableLen)
{
	int i;
	struct handleTable client;
	initHandleTable(&client);

	printf("#### sendResponse ####\n");

	switch(flag)
	{
		case 2: case 3: case 7: case 9: /*case 5: */
			printf("clientSocket: %d \n", clientSocket);
			sendToClient(clientSocket, pduBuffer, pduLen);
			break;

		case 4:
			for(i = 0; i < tableLen; i++)
			{
				client = (*table)[i];
				if(strcmp(client.handle, srcHandle) != 0 && !isEmpty(client))
					sendToClient(client.socketNum, pduBuffer, pduLen);
			}
			break;

		case 5: case 6: 
			// send is taken care of in createResponse
			break;

		case 11:
			sendToClient(clientSocket, pduBuffer, pduLen);
			sendAllHandlesFromTable(clientSocket, table, tableLen);
			sendHandleListACK(clientSocket);
			break;

		default:
			perror("#ERROR: createResponsePDU defaulted");
			// exit(-1);
			break;
	}
}

void sendAllHandlesFromTable(int clientSocket, struct handleTable** table, int tableLen)
{
	struct handleTable client;
	initHandleTable(&client);
	char* handle;
	uint8_t pduBuffer[MAXBUF];   		// data buffer for PDU 
	int pduLen = 0;
	int flag = 12;

	printf("#### sendAllHandlesFromTable ####");

	int i;
	for(i = 0; i < tableLen; i++)
	{
		client = (*table)[i];
		if(!isEmpty(client))
		{
			handle = getHandleByIndex(*table, tableLen, i);

			clearPDUBuffer(pduBuffer, MAXBUF);
			pduLen += addAByte(pduBuffer, flag, FLAG_INDEX);
			pduLen += addHandle(pduBuffer, handle, pduLen);

			// printf("handleLen: %d, handle: %s \n", (uint8_t)strlen(handle), handle);
			// printBuffer(pduBuffer, pduLen);
			sendToClient(clientSocket, pduBuffer, pduLen);
			pduLen = 0;
		}
	}
}

void sendHandleListACK(int clientSocket)
{
	uint8_t pduBuffer[MAXBUF];   		// data buffer for PDU 
	int pduLen = 0;
	int flag = 13;

	pduLen += addAByte(pduBuffer, flag, FLAG_INDEX);
	sendToClient(clientSocket, pduBuffer, pduLen);
}

int processClientToSend(int clientSocket, uint8_t* pduBuffer, uint8_t* dataBuffer, uint8_t* flag, int pduLen, 
	struct handleTable** table, int* tableLen, char* destHandle)
{
	if((findHandleIndex(*table, *tableLen, destHandle)) != -1)
	{
		// *clientSocket = getSocketByHandle(*table, *tableLen, destHandle);
		memcpy(pduBuffer, dataBuffer, pduLen);
	}
	else
	{
		*flag = 7;
		pduLen = addAByte(pduBuffer, *flag, FLAG_INDEX);
		pduLen += addHandle(pduBuffer, destHandle, pduLen);
		pduBuffer[pduLen] = '\0';
	}

	return pduLen;
}

void processAndSendClientToSend(int clientSocket, uint8_t* pduBuffer, uint8_t* dataBuffer, uint8_t* flag, int pduLen, 
	struct handleTable** table, int* tableLen, struct handleTable** destTable, int* destTableLen)
{
	int i;
	int pduLenSave = pduLen;
	int sendSocket = 0;

	for(i = 0; i < *destTableLen; i++)
	{
		if(!isEmpty((*destTable)[i]))
        {
			// memcpy(pduBuffer, &pduBuffer[curHeaderLen], curHeaderLen);

			pduLen = processClientToSend(clientSocket, pduBuffer, dataBuffer, flag, pduLenSave, table, tableLen, (*destTable)[i].handle);
			printBuffer(pduBuffer, pduLen);
			
			if((sendSocket = getSocketByIndex(*destTable, *destTableLen, i)) != -1)
				sendToClient(sendSocket, pduBuffer, pduLen);
			else
				sendToClient(clientSocket, pduBuffer, pduLen);
        }
	}
}

// void sendToClientNumHandles(int clientSocket, uint8_t* pduBuffer, int pduLen, struct handleTable* destTable, int destTableLen)
// {
// 	int i;

// 	for(i = 0; i < tableLen; i++)
// 	{
// 		if(!isEmpty(table[i]))
//         {
// 			printf("clientSocket:%d \n", destTable[i].socketNum);
// 			sendToClient(destTable[i].socketNum, pduBuffer, pduLen);
//         }
// 	}
// } 