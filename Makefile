# Makefile for CPE464 tcp test code
# written by Hugh Smith - April 2019

CC= gcc
CFLAGS= -g -Wall
LIBS = 

OBJS = networks.o gethostbyname.o pollLib.o safeUtil.o pdu.o handleTable.o

all:   cclient server

cclient: cclient.c $(OBJS)
	$(CC) $(CFLAGS) -o cclient cclient.c $(OBJS) $(LIBS)

server: server.c $(OBJS)
	$(CC) $(CFLAGS) -o server server.c $(OBJS) $(LIBS)

.c.o:
	gcc -c $(CFLAGS) $< -o $@ $(LIBS)

cleano:
	rm -f *.o

clean:
	rm -f server cclient *.o

server1:
	./server 6969

client11:
	./cclient s1_first unix3.csc.calpoly.edu 6969

client12:
	./cclient s1_second unix3.csc.calpoly.edu 6969

client13:
	./cclient s1_third unix3.csc.calpoly.edu 6969


server2:
	./server 8989

client21:
	./cclient s2_first unix3.csc.calpoly.edu 8989

client22:
	./cclient s2_second unix3.csc.calpoly.edu 8989

client23:
	./cclient s2_third unix3.csc.calpoly.edu 8989


server3:
	./server 9898

client31:
	./cclient s3_first unix3.csc.calpoly.edu 9898

client32:
	./cclient s3_second unix3.csc.calpoly.edu 9898

client33:
	./cclient s3_third unix3.csc.calpoly.edu 9898


