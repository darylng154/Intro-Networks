#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "window.h"
#include "pdu.h"

void initWindow(Window* window, uint32_t windowsize, int16_t buffersize)
{
    window->current = 0;
    window->lower = 0;
    window->upper = windowsize + window->lower;

    window->buffersize = buffersize;
    window->windowsize = windowsize;
    window->buffers = (uint8_t*) malloc(windowsize * MAXBUFSIZE);
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

uint8_t* getIndex(Window* window, int index)
{
    index = index % window->windowsize;
    return ((window->buffers) + (index * MAXBUFSIZE));
}

void copyDataAtIndex(uint8_t* dataBuffer, Window* window, int index)
{
    index = index % window->windowsize;
    memcpy(dataBuffer, getIndex(window, index), MAXBUFSIZE);
}

void addToWindow(Window* window, uint8_t* dataBuffer, int dataLen, int index)
{
    index = index % window->windowsize;
    memcpy(getIndex(window, index), dataBuffer, dataLen);
}

void printWindowFields(Window* window)
{
    printf("buffersize: %d | windowsize: %d current: %d | lower: %d | upper: %d \n",
    window->buffersize, window->windowsize, window->current, window->lower, window->upper);
}

void printWindow(Window* window)
{
    int i = 0;
    printWindowFields(window);
    for(i = 0; i < window->windowsize; i++)
    {
        i = i % window->windowsize;
        printf("window[%d]: \n", i);
        printBuffer(getIndex(window, i), window->buffersize);
        printf("\n");
    }
}