CC= gcc
CFLAGS= -g -Wall
LIBS = 

OBJS = window.o pdu.o

all:   window

window: test.c $(OBJS)
	$(CC) $(CFLAGS) -o test test.c $(OBJS) $(LIBS)

clean:
	rm *.o

make run:
	./test ../from.txt to.txt

make big

