#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "window.h"
#include "pdu.h"

void initWindow(Window* window, uint32_t windowsize, int16_t buffersize)
{
    window->isBuffered = 0;
    window->current = 0;
    window->lower = 0;
    window->upper = windowsize + window->lower;

    window->buffersize = buffersize;
    window->windowsize = windowsize;
    window->buffers = (struct dataBuffer*) calloc(sizeof(struct dataBuffer), windowsize);

    int i = 0;
    for(i = 0; i < windowsize; i++)
    {
        window->buffers[i].valid = 0;
        window->buffers[i].sequenceNum = 0;
    }
}

void cleanup(Window* window)
{
    free(window->buffers);
    free(window);
}

int getCurrent(Window* window)
{
    return window->current;
}

void setCurrent(Window* window, int current)
{
    window->current = current;
}

int getLower(Window* window)
{
    return window->lower;
}

void setLower(Window* window, int rrNum)
{
    window->lower = rrNum;
    window->upper = window->windowsize + window->lower;
}

int getUpper(Window* window)
{
    return window->upper;
}

struct dataBuffer* getIndex(Window* window, int index)
{
    index = index % window->windowsize;
    return &(window->buffers[index]);
}

void addToWindow(Window* window, uint8_t* dataBuffer, int dataLen, int index)
{
    memcpy(getIndex(window, index)->data, dataBuffer, dataLen);
}

void printWindowFields(Window* window)
{
    printf("buffersize: %d | windowsize: %d current: %d | lower: %d | upper: %d | isBuffered: %d \n",
    window->buffersize, window->windowsize, window->current, window->lower, window->upper, window->isBuffered);
}

void printBufferFields(Window* window, int index)
{
    printf("window[%d]: valid: %d, sequenceNum: %d \n", index, getIndex(window, index)->valid, getIndex(window, index)->sequenceNum);
}

void printWindow(Window* window)
{
    int i = 0;
    printWindowFields(window);
    for(i = 0; i < window->windowsize; i++)
    {
        printBufferFields(window, i);
        printBuffer(getIndex(window, i)->data, window->buffersize);
        printf("\n");
    }
}

void copyDataAtIndex(uint8_t* dataBuffer, Window* window, int index)
{
    memcpy(dataBuffer, getIndex(window, index)->data, MAXBUFSIZE);
}

void setValid(Window* window, int index, uint8_t valid)
{
    getIndex(window, index)->valid = valid;
}

uint8_t getValid(Window* window, int index)
{
    return getIndex(window, index)->valid;
}

void setSequenceNum(Window* window, int index, uint32_t sequenceNum)
{
    getIndex(window, index)->sequenceNum = sequenceNum;
}

uint8_t getSequenceNum(Window* window, int index)
{
    return getIndex(window, index)->sequenceNum;
}

void setIsBuffered(Window* window, int isBuffered)
{
    window->isBuffered = isBuffered;
}

int getIsBuffered(Window* window)
{
    return window->isBuffered;
}

int16_t getBuffersize(Window* window)
{
    return window->buffersize;
}

uint32_t getWindowsize(Window* window)
{
    return window->windowsize;
}

    