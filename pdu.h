#ifndef PDU_H
#define PDU_H

int sendPDU(int socketNumber, uint8_t * dataBuffer, int lengthOfData);
int recvPDU(int clientSocket, uint8_t * dataBuffer, int bufferLen);

int addPDUFlag(uint8_t* dataBuffer, int flag);
void parsePDUFlag(uint8_t* dataBuffer, int* flag);
void printPDUFlag(uint8_t* dataBuffer);

#endif