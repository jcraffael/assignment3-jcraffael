CC ?=$(CROSS_COMPILE)gcc

CFLAGS ?= -Wall -g

all: aesdsocket

default: aesdsocket

aesdsocket: aesdsocket.o
	$(CC) -o aesdsocket aesdsocket.o
aesdsocket.o: aesdsocket.c
	$(CC) $(CFLAGS) -c aesdsocket.c

clean:
	-rm -f *.o aesdsocket
