#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>

#include <math.h>

#define MAXBUF 1400

#include "pdu.h"
#include "handleTable.h"

int sendPDU(int socketNumber, uint8_t * dataBuffer, int lengthOfData)
{
    uint16_t pduLen = lengthOfData+2;
    uint8_t appPDU[pduLen];

    pduLen = htons(pduLen);

    memcpy(appPDU, &(pduLen), 2);
    memcpy(&(appPDU[2]), dataBuffer, lengthOfData);

    int sent;
    if((sent = send(socketNumber, appPDU, ntohs(pduLen), 0)) <0)
    {
        perror("#ERROR: sendPDU send()");
        exit(-1);
    }

    return sent;
}

int recvPDU(int clientSocket, uint8_t * dataBuffer, int bufferSize)
{
    // printf("\n");
    uint16_t pduLen = 0;
    clearPDUBuffer(dataBuffer, bufferSize);

    // 1st recv() = pdu header = payload/data len
    int recved = 0;
    if ((recved = recv(clientSocket, &pduLen, 2, MSG_WAITALL)) < 0)
	{
        if(errno == ECONNRESET)
        {
            recved = 0;
        }
        else
        {
            perror("#ERROR: appPDU revPDU recv()");
            exit(-1);
        }
	}
    else if(recved == 0)
    {
        // printf("\nConnection Closed.\n");
        return recved;
    }

    pduLen = ntohs(pduLen);     //note: pduLen = msgLen + PDUHeader(2 Bytes)

    if(pduLen > bufferSize && recved > bufferSize)
    {
        perror("#ERROR: appPDU dataBuffer size too small");
        exit(-1);
    }

    // 2nd recv() = payload/data
    if ((recved = recv(clientSocket, dataBuffer, pduLen-2, MSG_WAITALL)) < 0)
	{
		perror("#ERROR: appPDU revPDU recv()");
		exit(-1);
	}
    else if(recved == 0)
    {
        // printf("\nConnection Closed.\n");
        // return recved;
    }

    printf("socket: %d, pduLen: %d \n", clientSocket, recved);

    return recved;   
}

void printBuffer(uint8_t buffer[], int length)
{
    int i;

    if(buffer == NULL)
    {
        perror("#ERROR: buffer empty!!!!");
        exit(1);
    }

    for(i = 0; i < length; i++)
    {
        if(buffer[i] == '\0')
        {
            printf("\n\n");
            return;
        }
        else if(i % 4 == 0  && i != 0)
        {
            printf("\n");

            // if(i % 8 == 0)   // for spacing octets
            //     printf("\n");
        }

        printf("[%02d] 0x%0X ('%c')\t", i, buffer[i], (char)buffer[i]);
    }
    printf("\n");
}

void clearPDUBuffer(uint8_t* dataBuffer, int length)
{
    memset(dataBuffer, '\0', length);
}

int addAByte(uint8_t* dataBuffer, uint8_t byte, int curHeaderLen)
{
    memcpy(&(dataBuffer[(curHeaderLen)]), &(byte), 1);

    return 1;
}

uint8_t parseAByte(uint8_t* dataBuffer, int curHeaderLen)
{
    uint8_t byte = 0;
    memcpy(&byte, &(dataBuffer[curHeaderLen]), 1);
    return byte;
}

void printAByte(uint8_t* dataBuffer, int curHeaderLen)
{
    printf("AByte: %d \n", dataBuffer[curHeaderLen]);
}

int addHandle(uint8_t* dataBuffer, char* handle, int curHeaderLen)
{
    int handleLen = strlen(handle);
    memcpy(&(dataBuffer[(curHeaderLen)]), &(handleLen), 1);

    memcpy(&(dataBuffer[(curHeaderLen + 1)]), handle, handleLen);

    return handleLen + 1;
}

// NOTE: add int numHandles for %C multiple handles
int parseHandle(uint8_t* dataBuffer, char* handle, int curHeaderLen)
{
    int handleLen = 0;

    memcpy(&handleLen, &(dataBuffer[curHeaderLen]), 1);
			
    memcpy(handle, &(dataBuffer[curHeaderLen + 1]), handleLen);
    handle[handleLen] = '\0';

    return handleLen + 1;
}

int addMessage(uint8_t* dataBuffer, uint8_t* msgBuffer, int headerLen)
{
    int msgLen = strlen((char*)msgBuffer);

    memcpy(&(dataBuffer[(headerLen)]), msgBuffer, msgLen);
    dataBuffer[headerLen + msgLen] = '\0';
    msgLen++;   // include Null

    return msgLen;
}

int addNumHandlesInTable(uint8_t* dataBuffer, uint32_t numHandles, int curHeaderLen)
{
    uint32_t num = numHandles;
    num = htonl(numHandles);
    memcpy(&(dataBuffer[curHeaderLen]), &num, sizeof(num));

    return sizeof(numHandles);
}

void printNumHandlesInTable(uint8_t* dataBuffer, int curHeaderLen)
{
    uint32_t num = 0;
    memcpy(&num, &(dataBuffer[curHeaderLen]), sizeof(num));
    num = ntohl(num);
}

uint32_t parseNumHandlesInTable(uint8_t* dataBuffer, int curHeaderLen)
{
    uint32_t numHandles = 0;
    memcpy(&numHandles, &(dataBuffer[curHeaderLen]), sizeof(numHandles));
    numHandles = ntohl(numHandles);

    return numHandles;
}

int getNumSends(uint8_t* dataBuffer)
{
	int numSends = (int)strlen((char*)dataBuffer) / MAX_MSG;
    numSends++;

    return numSends;
}

void next200Msg(uint8_t* msgBuffer, uint8_t* stdinBuffer)
{
    memcpy(msgBuffer, stdinBuffer, MAX_MSG);
    msgBuffer[MAX_MSG] = '\0';  // null terminate msgBuffer

    if((int)strlen((char*)msgBuffer) < MAX_MSG)
    {
        msgBuffer[(int)strlen((char*)msgBuffer)] = '\0';
    }

    memcpy(stdinBuffer, &stdinBuffer[MAX_MSG], MAXBUF); // clear the read 200 chars

    fflush(stdin);
    // printf("#### AFTER next200Msg ####\n");
    // printf("msg: len: %d \n%s \n", (int)strlen((char*)msgBuffer), (char*)msgBuffer);
	// printf("stdin: len: %d \n%s \n", (int)strlen((char*)stdinBuffer), (char*)stdinBuffer);
}

void printBufferWith0(uint8_t buffer[], int length)
{
    int i;

    if(buffer == NULL)
    {
        perror("#ERROR: buffer empty!!!!");
        exit(1);
    }

    for(i = 0; i < length; i++)
    {
        if(i % 4 == 0  && i != 0)
        {
            printf("\n");
        }

        printf("[%02d] 0x%0X ('%c')\t", i, buffer[i], (char)buffer[i]);
    }
    printf("\n");
}

int parseNumHandles(uint8_t* dataBuffer, struct handleTable** table, int* tableLen, struct handleTable** destTable, int* destTableLen, int curHeaderLen, int numHandles)
{
	int i;
	char destHandle[MAX_HANDLE_LEN] = {'\0'};
	int socketNum = 0;

	for(i = 0; i < numHandles; i++)
	{
		curHeaderLen += parseHandle(dataBuffer, destHandle, curHeaderLen);
		socketNum = getSocketByHandle(*table, *tableLen, destHandle);
		*destTableLen = addHandleToTable(destTable, *destTableLen, socketNum, destHandle);
	}
	
	return curHeaderLen;
}