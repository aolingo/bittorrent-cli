all: bitclient


CLIBS=-pthread
CC=gcc
CPPFLAGS=
CFLAGS=-g

MAINOBJS=bitclient.o 

bitclient: $(MAINOBJS)
	$(CC) -o bitclient $(MAINOBJS)  $(CLIBS)


clean:
	rm -f *.o
	rm -f bitclient

