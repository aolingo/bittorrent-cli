// first commit testing, main file
#include "bitclient.h"

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

#include "bencode.h"

// TODO, argument validations
int isArgValid(char *torrentPath, char *outPath) {
  // https://docs.microsoft.com/en-us/windows/win32/fileio/naming-a-file
  printf("Input torrentName: %s outPath: %s\n", torrentPath, outPath);
  // https://www.tutorialspoint.com/c_standard_library/c_function_strstr.htm
  return 1;
}

int main(int argc, char **argv) {
  // Parsing command line arguments
  if (argc != 3) {
    printf("Incorrect number of arguments\n");
    return -1;
  }

  // TODO: Arguments validation
  char *torrentName = argv[1];
  char *outPath = argv[2];

  printf("Input torrentName: %s outPath: %s\n", torrentName, outPath);

  if (!isArgValid(torrentName, outPath)) {
    return -1;
  }

  be_node *node;
  node = load_be_node(torrentName);
  printf("node has type %d and has string %s\n", node->type, node->val.s);
  return 0;
}
