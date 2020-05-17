// first commit testing, main file
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char **argv) {

  // Parsing command line arguments
  if (argc != 3) {
    printf("Incorrect number of arguments, need 3\n");
    return -1;
  }

  printf("arguments are %s, %s, %s\n", argv[0], argv[1], argv[2]);
  return 0;
}
