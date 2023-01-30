#ifndef PDU_H
#define PDU_H

int sendPDU(int socketNumber, uint8_t * dataBuffer, int lengthOfData);
int recvPDU(int clientSocket, uint8_t * dataBuffer, int bufferLen);

void addChatHeader(uint8_t* dataBuffer, int* messageLen);

#endif