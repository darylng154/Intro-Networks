#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>

#include "handleTable.h"

int testAdd(struct handleTable** table, int* length)
{
    int i;
    for(i = 0; i < 3; i++)
    {
        printf("i: %d \t", i);
        *length = addHandleToTable(table, *length, -1, "test");
    }
    return *length; 
}

// main to test handleTable
int main()
{
    int length = 2;

    struct handleTable* table = NULL;
    initHandleTables(&table, length);

    length = addHandleToTable(&table, length, 5, "client1");
    length = addHandleToTable(&table, length, 10, "client2");
    printHandleTables(table, length);
    printf("index %d \n", findHandleIndex(table, length, "client1"));
    printHandleTable(table[findHandleIndex(table, length, "client1")]);

    printf("\nExpand Table Tests \n");
    length = addHandleToTable(&table, length, 6, "client5");
    printHandleTables(table, length);
    
    printf("\nExpand Table to 100+2 Tests \n");

    int i;
    for(i = 0; i < 100; i++)
    {   
        // printf("index: %d, length: %d ", i, length);
        length = addHandleToTable(&table, length, 6, "clientTest");
    }
    printf("\n");
    printHandleTables(table, length);

    printf("table socket test: %d \n", table[0].socketNum);

    printf("\nSecond Table Tests \n");
    int length2 = 3;
    struct handleTable* table2 = NULL;
    initHandleTables(&table2, length2);

    // handleTableCpy(table2, table, length);
    length2 = addHandleToTable(&table2, length2, 6, "client5");
    printHandleTables(table2, length2);
    
    printf("\nSwap Entry Tests \n");
    swapHandles(&table2[2], &table2[1]);
    printHandleTables(table2, length2);
    printf("index %d \n", findSocketNumIndex(table, length, 10));
    printHandleTable(table[findSocketNumIndex(table, length, 10)]);

    printf("\nDelete Entry Tests \n");
    printf("index %d \n", findHandleIndex(table2, length2, "client1"));
    if(findHandleIndex(table2, length2, "client1") != -1)
    {
        printHandleTable(table[findHandleIndex(table2, length2, "client1")]);
        deleteHandleAtIndex(table2, length2, findHandleIndex(table2, length2, "client1"));
        printHandleTables(table2, length2);
    }

    printf("\nSwap Ptr Tests \n");
    free(table2);
    table2 = table;
    printHandleTables(table2, length);


    length = testAdd(&table, &length);
    printHandleTables(table, length);

    free(table);

    return 0;
}