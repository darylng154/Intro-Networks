#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>

#include <math.h>

#include "checksum.h"
#include "cpe464.h"
#include "networks.h"
#include "pollLib.h"

#include "pdu.h"

int sendPDU(Connection* connection, uint8_t* dataBuffer, int dataLen, uint32_t sequenceNum, uint8_t flag)
{
    int16_t pduLen = 0;
    uint8_t pduBuffer[MAXBUF] = {'\0'};

    if(dataLen > 0)
    {
        memcpy(&(pduBuffer[HEADERSIZE]), dataBuffer, dataLen);
    }

    pduLen = createHeader(pduBuffer, sequenceNum, flag, dataLen);

    int sent;
    if((sent = sendtoErr(connection->socketNum, pduBuffer, pduLen, 0, (struct sockaddr*) &(connection->remote), connection->length)) < 0)
    {
        perror("#ERROR: sendPDU sendtoErr == -1");
        exit(-1);
    }

    printf("sendPDU pduLen: %d sent: %d \n", pduLen, sent);
    printBuffer(pduBuffer, pduLen);
    printf("\n");

    return sent;
}

int recvPDU(Connection* connection, uint8_t* dataBuffer, int bufferSize, uint32_t* sequenceNum, uint8_t* flag)
{
    int16_t pduLen = 0;
    uint8_t pduBuffer[MAXBUF] = {'\0'};

    if ((pduLen = recvfromErr(connection->socketNum, pduBuffer, bufferSize, 0, (struct sockaddr*) &(connection->remote), &(connection->length))) < 0)
	{
		perror("#ERROR: recvPDU recvfromErr == -1");
		exit(-1);
	}

    printf("recvPDU pduLen: %d \n", pduLen);
    printBuffer(pduBuffer, pduLen);

    pduLen = parseHeader(pduBuffer, sequenceNum, flag, pduLen);

    if(pduLen > 0)
    {
        memcpy(dataBuffer, &(pduBuffer[HEADERSIZE]), pduLen);

        printf("parseHeader pduLen: %d \ndatabuffer: \n", pduLen);
        printBuffer(dataBuffer, pduLen);
    }

    printf("sequence#: %d, flag: %d \n\n", *sequenceNum, *flag);

    return pduLen;
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
        if(i % 4 == 0  && i != 0)
        {
            printf("\n");
        }

        if((char)buffer[i] == '\n')
            printf("[%02d] 0x%0X ('\\n')\t", i, (char)buffer[i]);
        else
            printf("[%02d] 0x%0X ('%c')\t", i, buffer[i], (char)buffer[i]);
    }
    printf("\n");
}

int createHeader(uint8_t* pduBuffer, uint32_t sequenceNum, uint8_t flag, int dataLen)
{
    int curHeaderLen = 0;
    uint16_t checksum = 0;
    sequenceNum = htonl(sequenceNum);

    int16_t pduLen = sizeof(sequenceNum) + sizeof(checksum) + sizeof(flag) + dataLen;

    memcpy(pduBuffer, &(sequenceNum), sizeof(sequenceNum));
    curHeaderLen = sizeof(sequenceNum) + sizeof(checksum);

    memcpy(&(pduBuffer[curHeaderLen]), &(flag), sizeof(flag));
    curHeaderLen += sizeof(flag);

    memset(&(pduBuffer[sizeof(sequenceNum)]), 0, sizeof(checksum));
    checksum = in_cksum((unsigned short*)pduBuffer, pduLen);
    memcpy(&(pduBuffer[sizeof(sequenceNum)]), &(checksum), sizeof(checksum));

    return pduLen;
}

int parseHeader(uint8_t* pduBuffer, uint32_t* sequenceNum, uint8_t* flag, int dataLen)
{
    int curHeaderLen = 0;
    int returnValue = 0;

    if(in_cksum((unsigned short*)pduBuffer, dataLen) != 0)
    {
        returnValue = CRC_ERROR;
    }
    else
    {
        memcpy(sequenceNum, pduBuffer, sizeof(*sequenceNum));
        *sequenceNum = ntohl(*sequenceNum);

        curHeaderLen = sizeof(*sequenceNum) + sizeof(uint16_t);
        memcpy(flag, &(pduBuffer[curHeaderLen]), sizeof(*flag));

        returnValue = dataLen - HEADERSIZE;
    }

    return returnValue;
}

int addString(uint8_t* dataBuffer, char* handle, int curHeaderLen)
{
    int handleLen = strlen(handle);
    memcpy(&(dataBuffer[(curHeaderLen)]), &(handleLen), 1);

    memcpy(&(dataBuffer[(curHeaderLen + 1)]), handle, handleLen);

    return handleLen + 1;
}

int parseString(uint8_t* dataBuffer, char* handle, int curHeaderLen)
{
    int handleLen = 0;

    memcpy(&handleLen, &(dataBuffer[curHeaderLen]), 1);

    printf("parseHandle handleLen: %d ", handleLen);
			
    memcpy(handle, &(dataBuffer[curHeaderLen + 1]), handleLen);
    handle[handleLen] = '\0';

    printf(", handle: %s \n", handle);

    return handleLen + 1;
}