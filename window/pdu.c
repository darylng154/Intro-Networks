#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>

#include <math.h>

#include "pdu.h"

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