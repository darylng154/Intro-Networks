#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>

#define MAXBUF 1400
#define FLAG_INDEX 2

#include "pdu.h"

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
    printf("\n");
    uint16_t pduLen = 0;

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
        printf("\nConnection Closed.\n");
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
        printf("\nConnection Closed.\n");
        return recved;
    }

    return recved;   
}

int addPDUFlag(uint8_t* dataBuffer, int flag)
{
    int flagLen = 1;
    memcpy(&(dataBuffer[FLAG_INDEX]), &(flag), flagLen);

    return flagLen;
}

void parsePDUFlag(uint8_t* dataBuffer, int* flag)
{
    memcpy(flag, &(dataBuffer[FLAG_INDEX]), 1);
}

void printPDUFlag(uint8_t* dataBuffer)
{
    printf("flag = %d \n", dataBuffer[FLAG_INDEX]);
}
