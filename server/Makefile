CC=$(CROSS_COMPILE)g++

CFLAGS = -Wall -g

all: aesdsocket

default: aesdsocket

aesdsocket: aesdsocket.o
	$(CC) -o aesdsocket aesdsocket.o
aesdsocket.o: aesdsocket.cpp
	$(CC) $(CFLAGS) -c aesdsocket.cpp

clean:
	-rm -f *.o aesdsocket
