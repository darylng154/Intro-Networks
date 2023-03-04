#ifndef HANDLETABLE_H
#define HANDLETABLE_H

struct handleTable
{
    int socketNum;
    char handle[100];
};

void initHandleTable(struct handleTable* entry);
void initHandleTables(struct handleTable** table, int length);

int isEmpty(struct handleTable entry);

int findHandleIndex(struct handleTable* table, int length, char* handle);
int findSocketNumIndex(struct handleTable* table, int length, int socketNum);

int addHandleToTable(struct handleTable** table, int length, int socketNum, char* handle);
void swapHandles(struct handleTable* entry1, struct handleTable* entry2);
void deleteHandleAtIndex(struct handleTable* table, int length, int index);
void handleTableCpy(struct handleTable* newTable, struct handleTable* table, int length);

int numEntries(struct handleTable* table, int length);

void printHandleTable(struct handleTable table);
void printHandleTables(struct handleTable* table, int length);

int getSocketByIndex(struct handleTable* table, int tableLen, int index);
char* getHandleByIndex(struct handleTable* table, int tableLen, int index);

int getSocketByHandle(struct handleTable* table, int tableLen, char* handle);

#endif