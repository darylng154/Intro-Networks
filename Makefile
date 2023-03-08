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

serverfrom:
	./server 0.1 5455

clientsmallfrom:
	./rcopy smallfrom.txt smallto.txt 5 14 0.4 unix3.csc.calpoly.edu 5455

clientfrom:
	./rcopy from.txt to.txt 5 14 0.3 unix3.csc.calpoly.edu 5455

clientbigfrom:
	./rcopy bigfrom.txt bigto.txt 5 14 0.1 unix3.csc.calpoly.edu 5455

clientmid:
	./rcopy bigfrom.txt midto.txt 10 1000 0.1 unix3.csc.calpoly.edu 5455

clientempty:
	./rcopy empty.txt to.txt 5 14 0 unix3.csc.calpoly.edu 5455


