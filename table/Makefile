# Makefile for CPE464 tcp test code
# written by Hugh Smith - April 2019

CC= gcc
CFLAGS= -g -Wall
LIBS = 

OBJS = handleTable.o

all:   table

table: main.c $(OBJS)
	$(CC) $(CFLAGS) -o main main.c $(OBJS) $(LIBS)

clean:
	rm handleTable
	rm *.o

make run:
	./main


