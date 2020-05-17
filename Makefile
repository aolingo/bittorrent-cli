all: main


CLIBS=-pthread
CC=gcc
CPPFLAGS=
CFLAGS=-g

MAINOBJS=bittorrentcli.o 

main: $(MAINOBJS)
	$(CC) -o main $(MAINOBJS)  $(CLIBS)



clean:
	rm -f *.o
	rm -f main

