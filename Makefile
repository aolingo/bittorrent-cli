# all: bitclient bencode
CLIBS=-pthread
CC=gcc
CFLAGS=-g -Wall


SRC= bencode.c  bitclient.c
OBJ=$(SRC:.c=.o)
BIN=bitclient

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJ) 


%.o:%.c
	$(CC) -c $(CFLAGS) -o $@ $<  

$(SRC):

clean:
	rm -rf $(OBJ) $(BIN)

# MAINOBJS=bitclient.o 

# bitclient: $(MAINOBJS)
# 	$(CC) -o bitclient $(MAINOBJS)  $(CLIBS)


# clean:
# 	rm -f *.o
# 	rm -f bitclient

