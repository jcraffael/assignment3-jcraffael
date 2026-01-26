
ifdef CROSS_COMPILE
	CC=$(CROSS_COMPILE)gcc
else
	CC=gcc
endif
CFLAGS = -Wall -g

all: writer
writer: writer.o
	$(CC) -o writer writer.o
writer.o:
	$(CC) $(CFLAGS) -c writer.c

clean:
	-rm -f *.o writer
