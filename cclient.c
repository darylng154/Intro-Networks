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

#define MAXBUF 1400
#define DEBUG_FLAG 1
#define COMMAND_LEN 2

void sendToServer(int socketNum, uint8_t* sendBuf, int sendLen);
int readFromStdin(uint8_t * buffer);
void checkArgs(int argc, char * argv[]);
void clientControl(int serverSocket);
void recvFromServer(int serverSocket);
int processCommand(uint8_t* dataBuffer);

int main(int argc, char * argv[])
{
	int socketNum = 0;         //socket descriptor
	
	checkArgs(argc, argv);

	/* set up the TCP Client socket  */
	socketNum = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);

	clientControl(socketNum);
	
	close(socketNum);
	
	return 0;
}

void sendToServer(int socketNum, uint8_t* sendBuf, int sendLen)
{
// 	uint8_t sendBuf[MAXBUF];   //data buffer
// 	int sendLen = 0;        //amount of data to send
	int sent = 0;            //actual amount of data sent/* get the data and send it   */
	
	// sendLen = readFromStdin(sendBuf);
	printf("read: %sstring len: %d (including null)\n", sendBuf, sendLen);
	
	sent = sendPDU(socketNum, sendBuf, sendLen);
	if (sent < 0)
	{
		perror("send call");
		exit(-1);
	}

	printf("Amount of data sent is: %d\n\n", sent);
}

int readFromStdin(uint8_t * buffer)
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

void clientControl(int serverSocket)
{
	int readySocket = 0;
	uint8_t dataBuffer[MAXBUF];   //data buffer
	int messageLen = 0;        //amount of data to send
	int flag = 0;
	
	setupPollSet();
	addToPollSet(serverSocket);
	addToPollSet(STDIN_FILENO);

	while(1)
	{
		//poll & block forever
		readySocket = pollCall(-1);

		if(readySocket == STDIN_FILENO)		// socket ready to send
		{
			messageLen = readFromStdin(dataBuffer);
			flag = processCommand(dataBuffer);
			printf("client send flag: %d \n", flag);
			
			// build pdu based on input
			addPDUFlag(dataBuffer, flag);
			printPDUFlag(dataBuffer);

			sendToServer(serverSocket, dataBuffer, messageLen);
		}
		else if(readySocket == serverSocket)	// socket ready to recv
		{
			recvFromServer(serverSocket);
		}
	}

	printf("cclient exited while loop \n");
}

void recvFromServer(int serverSocket)
{
	uint8_t dataBuffer[MAXBUF];   	//data buffer
	int recved = 0;        			//amount of data to send

	recved = recvPDU(serverSocket, dataBuffer, MAXBUF /*, (uint8_t*)&flag*/);
	if(recved > 0)
	{
		printf("Message received on socket %d, length: %d Data: %s\n", serverSocket, recved, dataBuffer);
		printf("(note the length is %d because of the null)\n", recved);
		// printflag("", flag);
		printf("\n\n");
	}
	else if(recved < 0)
	{
		perror("#ERROR: myClient.c recvFromServer recvPDU < 0");
		exit(-1);
	}
	if(recved == 0)		//checks if socket is closed
	{
		perror("ERROR#: Server closed");
		exit(-1);
	}
}

int processCommand(uint8_t* dataBuffer)
{
	int flag = 0;

	char command[MAXBUF];
	memcpy(command, dataBuffer, COMMAND_LEN);
	command[COMMAND_LEN+1] = '\0';

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

	printf("command: %c, flag: %d \n", command[1], flag);

	return flag;
}