#ifndef PDU_H
#define PDU_H

#include "networks.h"

#define MAXBUF 1407     // max buffer = 1400 | header size = 7 Bytes
#define HEADERSIZE 7
#define MAXFILENAME 100
#define ONE_SEC 1000
#define TEN_SEC 10000
#define MAXRETRIES 10

enum FLAG
{
    DATA = 3, RR = 5, SREJ = 6, FNAME = 7, FNAME_ACK = 8,  END_OF_FILE = 9, EOF_ACK = 10,
    CRC_ERROR = -1
};

int sendPDU(Connection* connection, uint8_t* dataBuffer, int dataLen, uint32_t sequenceNum, uint8_t flag);
int recvPDU(Connection* connection, uint8_t* dataBuffer, int bufferSize, uint32_t* sequenceNum, uint8_t* flag);

void printBuffer(uint8_t buffer[], int length);

int createHeader(uint8_t* pduBuffer, uint32_t sequenceNum, uint8_t flag, int dataLen);
int parseHeader(uint8_t* pduBuffer, uint32_t* sequenceNum, uint8_t* flag, int dataLen);

int addString(uint8_t* dataBuffer, char* handle, int curHeaderLen);
int parseString(uint8_t* dataBuffer, char* handle, int curHeaderLen);

#endif