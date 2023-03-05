# Makefile for CPE464 tcp and udp test code
# updated by Hugh Smith - April 2023

# all target makes UDP test code
# tcpAll target makes the TCP test code


CC= gcc
CFLAGS= -g -Wall
LIBS = 

OBJS = networks.o gethostbyname.o pollLib.o safeUtil.o pdu.o window.o

#uncomment next two lines if your using sendtoErr() library
LIBS += libcpe464.2.21.a -lstdc++ -ldl
CFLAGS += -D__LIBCPE464_


all: udpAll

udpAll: rcopy server

rcopy: rcopy.c $(OBJS) 
	$(CC) $(CFLAGS) -o rcopy rcopy.c $(OBJS) $(LIBS)

server: server.c $(OBJS) 
	$(CC) $(CFLAGS) -o server server.c  $(OBJS) $(LIBS)

.c.o:
	gcc -c $(CFLAGS) $< -o $@ $(LIBS)

cleano:
	rm -f *.o

clean:
	rm -f myServer myClient rcopy server *.o


server1:
	./server 0 6969 v

client1:
	./rcopy from.txt to.txt 100 1000 0 unix3.csc.calpoly.edu 6969 v

bigclient1:
	./rcopy bigfrom.txt bigto.txt 100 1000 0 unix3.csc.calpoly.edu 6969 v


server2:
	./server 0 8989 v

client2:
	./rcopy from.txt to.txt 100 1000 0 unix3.csc.calpoly.edu 8989 v

bigclient2:
	./rcopy bigfrom.txt bigto.txt 100 1000 0 unix3.csc.calpoly.edu 8989 v


server3:
	./server 0 9898 v

client3:
	./rcopy from.txt to.txt 100 1000 0 unix3.csc.calpoly.edu 9898 v


serverDrop:
	./server .88 7899 v

clientDrop:
	./rcopy from.txt to.txt 100 1000 .88 unix3.csc.calpoly.edu 7899 v

serverfrom:
	./server 0 5454 v

clientfrom:
	./rcopy from.txt to.txt 3 14 0 unix3.csc.calpoly.edu 5454 v

serverbigfrom:
	./server 0 5454 v

clientbigfrom:
	./rcopy bigfrom.txt bigto.txt 3 14 0 unix3.csc.calpoly.edu 5454 v

serverempty:
	./server 0 5454 v

clientempty:
	./rcopy empty.txt to.txt 3 14 0 unix3.csc.calpoly.edu 5454 v


