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

// void recvFromClient(int clientSocket);
void recvFromClient(int clientSocket, uint8_t* dataBuffer);
int checkArgs(int argc, char *argv[]);
void serverControl(int mainServerSocket);
void addNewClient(int serverSocket, int debugFlag);
// void processClient(int clientSocket);
void processClient(int clientSocket, uint8_t* dataBuffer);
void replyToClient(int clientSocket, uint8_t* dataBuffer, int sendLen);

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

void recvFromClient(int clientSocket, uint8_t* dataBuffer)
{
	// uint8_t dataBuffer[MAXBUF];
	int messageLen = 0;
	
	//now get the data from the client_socket
	if ((messageLen = recvPDU(clientSocket, dataBuffer, MAXBUF)) < 0)
	{
		perror("#ERROR: myServer.c recvFromClient recvPDU");
		exit(-1);
	}

	if (messageLen > 0)
	{
		printf("Message received on socket %d, length: %d Data: %s\n", clientSocket, messageLen, dataBuffer);
		printf("(note the length is %d because of the null)\n", messageLen);

		replyToClient(clientSocket, dataBuffer, messageLen);
	}
	if(messageLen == 0 || strcmp((char*)dataBuffer, "exit") == 0)		//checks if socket is closed first
	{
		removeFromPollSet(clientSocket);
		close(clientSocket);
	}
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

	char* handle = "\0";
	int tableLen = 2;
    struct handleTable* table = NULL;
    initHandleTables(&table, tableLen);

	uint8_t dataBuffer[MAXBUF];

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
			processClient(readySocket, dataBuffer);
		}
	}

	free(table);
}

void addNewClient(int serverSocket, int debugFlag)
{
	int clientSocket = 0;
	clientSocket = tcpAccept(serverSocket, debugFlag);
	addToPollSet(clientSocket);

	// *tableLen = addHandle(table, *tableLen, clientSocket, handle);
	// printHandleTables(*table, *tableLen);
}

void processClient(int clientSocket, uint8_t* dataBuffer)
{
	recvFromClient(clientSocket, dataBuffer);
}

void replyToClient(int clientSocket, uint8_t* dataBuffer, int sendLen)
{
	// uint8_t dataBuffer[MAXBUF];
	int sent = 0;

	sent = sendPDU(clientSocket, dataBuffer, sendLen/*, flag*/);	//echo back to client
	if(sent < 0)
	{
		perror("#ERROR: myClient.c sendToServer sendPDU");
		exit(-1);
	}
	printf("echo: %s string len: %d (including null)\n", dataBuffer, sendLen);
	printf("Amount of data sent is: %d\n", sent);
} 