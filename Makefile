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
	./server 0.3 5455 v

clientsmallfrom:
#	./rcopy ./testcases/smallfrom.txt ./testcases/smallto.txt 5 14 0.4 unix3.csc.calpoly.edu 5455
	./rcopy ./testcases/smallfrom.txt ./testcases/smallto.txt 10 20 0.4 unix3.csc.calpoly.edu 5455

clientfrom:
	./rcopy ./testcases/from.txt ./testcases/to.txt 5 14 0.3 unix3.csc.calpoly.edu 5455 v
#	./rcopy ./testcases/from.txt ./testcases/to.txt 5 50 0.3 unix3.csc.calpoly.edu 5455

clientbigfrom:
	./rcopy ./testcases/bigfrom.txt ./testcases/bigto.txt 5 14 0.1 unix3.csc.calpoly.edu 5455 v

servermid:
	./server 0.2 5455 v

clientmid:
	./rcopy ./testcases/mid ./testcases/midto 10 1000 0.2 unix3.csc.calpoly.edu 5455 v

serverbig50:
	./server 0.1 5455 v

clientbig50:
	./rcopy ./testcases/big ./testcases/big50to 50 1000 0.1 unix3.csc.calpoly.edu 5455 v

serverbig5:
	./server 0.15 5455

clientbig5:
	./rcopy ./testcases/big ./testcases/big5to 5 1000 0.15 unix3.csc.calpoly.edu 5455

clientempty:
	./rcopy ./testcases/empty.txt ./testcases/emptyto.txt 5 14 0 unix3.csc.calpoly.edu 5455

clientfile1:
	./rcopy ./testcases/file1 ./testcases/file1to 10 1000 0 unix3.csc.calpoly.edu 5455


