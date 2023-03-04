#ifndef PDU_H
#define PDU_H

#define MAX_HANDLE_LEN 101   // 100 + 1 null for buffer
#define MAX_MSG 199 // 200 includes NULL @ end
#define MAX_DEST_HANDLES 9

#define PDU_LEN_BYTES 2
#define FLAG_INDEX 0
#define HANDLE_LENGTH_INDEX 1

int sendPDU(int socketNumber, uint8_t * dataBuffer, int lengthOfData);
int recvPDU(int clientSocket, uint8_t * dataBuffer, int bufferLen);

void printBuffer(uint8_t buffer[], int length);

void clearPDUBuffer(uint8_t* dataBuffer, int length);
int addAByte(uint8_t* dataBuffer, uint8_t flag, int curHeaderLen);
uint8_t parseAByte(uint8_t* dataBuffer, int curHeaderLen);
void printAByte(uint8_t* dataBuffer, int curHeaderLen);

int addHandle(uint8_t* dataBuffer, char* handle, int curHeaderLen);
// NOTE: add int numHandles for %C multiple handles
int parseHandle(uint8_t* dataBuffer, char* handle, int curHeaderLen);

int addMessage(uint8_t* dataBuffer, uint8_t* msgBuffer, int headerLen);

int addNumHandlesInTable(uint8_t* dataBuffer, uint32_t numHandles, int curHeaderLen);
void printNumHandlesInTable(uint8_t* dataBuffer, int curHeaderLen);
uint32_t parseNumHandlesInTable(uint8_t* dataBuffer, int curHeaderLen);

int getNumSends(uint8_t* dataBuffer);
void next200Msg(uint8_t* msgBuffer, uint8_t* stdinBuffer);

void printBufferWith0(uint8_t buffer[], int length);

#endif