#ifndef WINDOW_H
#define WINDOW_H

#include <stdint.h>

#define MAXBUFSIZE 1400

typedef struct window Window;

struct dataBuffer
{
    uint8_t valid;
    uint32_t sequenceNum;
    uint32_t dataLen;
    uint8_t data[MAXBUFSIZE];
};

struct window
{
    int isBuffered;
	int current;
    int lower;
    int upper;
    
    int16_t buffersize;
    uint32_t windowsize;
    struct dataBuffer* buffers;
};

void initWindow(Window* window, uint32_t windowsize, int16_t buffersize);
void cleanup(Window* window);
int getCurrent(Window* window);
void setCurrent(Window* window, int current);
int getLower(Window* window);
void setLower(Window* window, int rrNum);
int getUpper(Window* window);
struct dataBuffer* getIndex(Window* window, int index);
void addToWindow(Window* window, uint8_t* dataBuffer, int dataLen, int index);
void printWindowFields(Window* window);
void printBufferFields(Window* window, int index);
void printWindow(Window* window);
int copyDataAtIndex(uint8_t* dataBuffer, Window* window, int index);
void setValid(Window* window, int index, uint8_t valid);
uint8_t getValid(Window* window, int index);
void setSequenceNum(Window* window, int index, uint32_t sequenceNum);
uint8_t getSequenceNum(Window* window, int index);
void setDataLen(Window* window, int index, uint32_t dataLen);
uint32_t getDataLen(Window* window, int index);
void setIsBuffered(Window* window, int isBuffered);
int getIsBuffered(Window* window);
int16_t getBuffersize(Window* window);
uint32_t getWindowsize(Window* window);

#endif