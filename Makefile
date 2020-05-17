all: main


CLIBS=-pthread
CC=gcc
CPPFLAGS=
CFLAGS=-g

MAINOBJS=main.o 

main: $(MAINOBJS)
	$(CC) -o main $(MAINOBJS)  $(CLIBS)



clean:
	rm -f *.o
	rm -f main

