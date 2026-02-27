CC ?=$(CROSS_COMPILE)gcc

CFLAGS ?= -Wall -g

LDFLAGS ?=
all: aesdsocket

default: aesdsocket

aesdsocket: aesdsocket.o
	$(CC) -o aesdsocket aesdsocket.o $(LDFLAGS)
aesdsocket.o: aesdsocket.c
	$(CC) $(CFLAGS) -c aesdsocket.c

clean:
	-rm -f *.o aesdsocket
