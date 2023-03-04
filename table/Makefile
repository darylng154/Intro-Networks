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


