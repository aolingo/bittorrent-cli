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

  printf("Torrent filepath: %s outPath: %s\n", torrentName, outPath);

  if (!isArgValid(torrentName, outPath)) {
    return -1;
  }

  be_node *node;
  node = load_be_node(torrentName);

  // type 3 dict
  be_dict *tempDict = node->val.d;
  printf("announce key %s \n", tempDict->key);
  printf("tracker url %s\n", (tempDict->val)->val.s);

  be_dict temp1 = node->val.d[1];
  printf("temp1 key %s \n", temp1.key);
  printf("temp1 val %s\n", (temp1.val)->val.s);

  be_dict temp2 = node->val.d[2];
  printf("temp2 key %s \n", temp2.key);
  printf("temp2 val %s\n", (temp2.val)->val.s);

  // temp1 = node->val.d[3];
  // printf("temp1 key %s \n", temp1.key);
  // printf("temp1 val %s\n", (temp1.val)->val.s);

  // temp1 = node->val.d[4];
  // printf("temp1 key %s \n", temp1.key);
  // printf("temp1 val %s\n", (temp1.val)->val.s);

  // temp1 = node->val.d[5];
  // printf("temp1 key %s \n", temp1.key);
  // printf("temp1 val %s\n", (temp1.val)->val.s);
  return 0;
}
