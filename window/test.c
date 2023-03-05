#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "window.h"

#define WINDOWSIZE 5
#define BUFFERSIZE 14

char VERBOSE = 'v';

int writeToFile(int toFile, uint8_t* dataBuffer, int dataLen)
{
    int writeLen = 0;

    if((writeLen = write(toFile, dataBuffer, dataLen)) == -1)
    {
        if(VERBOSE == 'v')
            perror("Error on writing to toFile");
            
        exit(-1);
    }

    return writeLen;
}

int main (int argc, char *argv[])
{
    Window* window = (Window*) calloc(1, sizeof(Window));
    int fromFile = -1;
    int toFile = -1;
    uint8_t dataBuffer[MAXBUFSIZE];
    uint8_t writeBuffer[MAXBUFSIZE];
    int readLen = 0;
    int writeLen = 0;
    int index = 0;

    initWindow(window, WINDOWSIZE, BUFFERSIZE);

    // printWindow(window);

    if((fromFile = open(argv[1], O_RDONLY)) == -1)
    {
        printf("Error: file %s not found. \n", argv[1]);
        exit(-1);
    }

    if((toFile = open(argv[2], O_CREAT | O_TRUNC | O_WRONLY, 0600)) == -1)
    {
        printf("Error: file %s not found. \n", argv[2]);
        exit(-1);
    }

    index = 0;
    while((readLen = read(fromFile, dataBuffer, BUFFERSIZE)) != 0 && getCurrent(window) != getUpper(window))
    {
        addToWindow(window, dataBuffer, BUFFERSIZE, index);
        setCurrent(window, getCurrent(window) + 1);

        copyDataAtIndex(writeBuffer, window, index);
        writeLen = writeToFile(toFile, writeBuffer, readLen);
        index++;

        setLower(window, getLower(window) + 1);

        printf("\n####################################################################\n");
        printf("readLen: %d | writeLen: %d | index: %d \n", readLen, writeLen, index);
        printWindow(window);
    }

    printf("\n####################################################################\n");
    printf("### Out of reading loop ###\n");
    printWindow(window);


    cleanup(window);

    return 0;
}