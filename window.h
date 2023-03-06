#ifndef WINDOW_H
#define WINDOW_H

#include <stdint.h>

#define MAXBUFSIZE 1400

typedef struct window Window;

struct window
{
	int current;
    int lower;
    int upper;
    
    int16_t buffersize;
    uint32_t windowsize;
    uint8_t* buffers;
};

void initWindow(Window* window, uint32_t windowsize, int16_t buffersize);
void cleanup(Window* window);
int getCurrent(Window* window);
void setCurrent(Window* window, int current);
int getLower(Window* window);
void setLower(Window* window, int rrNum);
int getUpper(Window* window);
uint8_t* getIndex(Window* window, int index);
void copyDataAtIndex(uint8_t* dataBuffer, Window* window, int index);
void addToWindow(Window* window, uint8_t* dataBuffer, int dataLen, int index);
void printWindowFields(Window* window);
void printWindow(Window* window);


#endif