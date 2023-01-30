#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>

#include "handleTable.h"

void initHandleTable(struct handleTable* entry)
{
    entry->socketNum = 0;
    strcpy(entry->handle, "\0");
}

void initHandleTables(struct handleTable** table, int length)
{
    *table = (struct handleTable*)malloc(sizeof(struct handleTable) * length);

    int i;
    for(i = 0; i < length; i++)
    {
        initHandleTable(&(*table)[i]);
    }
}

int isEmpty(struct handleTable entry)
{
    int empty = 1;
    if(entry.socketNum  != 0 && strcmp(entry.handle, "\0"))
    {
        empty = 0;
    }

    return empty;
}

int findHandleIndex(struct handleTable* table, int length, char* handle)
{
    int i;
    for(i = 0; i < length; i++)
    {
        if(!strcmp(table[i].handle, handle))
            return i;
    }

    return -1;
}

int findSocketNumIndex(struct handleTable* table, int length, int socketNum)
{
    int i;
    for(i = 0; i < length; i++)
    {
        if(table[i].socketNum == socketNum)
            return i;
    }

    return -1;
}

int addHandle(struct handleTable** table, int length, int socketNum, char* handle)
{
    int i;
    for(i = 0; i < length; i++)
    {
        if(isEmpty((*table)[i]))
        {
            (*table)[i].socketNum = socketNum;
            strcpy((*table)[i].handle, handle);
            return length;
        }
    }

    int newLength = length*2;
    // doubles handleTable
    *table = realloc(*table, sizeof(struct handleTable) * newLength);
    (*table)[i].socketNum = socketNum;
    strcpy((*table)[i].handle, handle);

    return newLength;

    // perror("!!!addHandle No Empty Index!!!");
    // exit(-1);
}

void swapHandles(struct handleTable* entry1, struct handleTable* entry2)
{
    struct handleTable temp;
    initHandleTable(&temp);

    temp = *entry1;
    *entry1 = *entry2;
    *entry2 = temp;
}

void deleteHandleAtIndex(struct handleTable* table, int length, int index)
{
    if(index != -1)
        initHandleTable(&table[index]);
    else
    {
        perror("!!!deleteHandleAtIndex Entry Does Not Exist!!!");
        exit(-1);
    }
}

void handleTableCpy(struct handleTable* newTable, struct handleTable* table, int length)
{
    int i;
    for(i = 0; i < length; i++)
    {
        newTable[i].socketNum = table[i].socketNum;
        strcpy(newTable[i].handle, table[i].handle);
    }
}


int numEntries(struct handleTable* table, int length)
{
    int i;
    int count = 0;
    for(i = 0; i < length; i++)
    {
        if(!isEmpty(table[i]))
            count++;
    }
    return count;
}

void printHandleTable(struct handleTable table)
{
    printf("socketNum: %d, handle: %s \n", table.socketNum, table.handle);
}

void printHandleTables(struct handleTable* table, int length)
{
    printf("length: %d \t entries: %d \n", length, numEntries(table, length));
    int i;
    for(i = 0; i < length; i++)
    {
        printf("[%d] ", i);
        printHandleTable(table[i]);
    }
}

