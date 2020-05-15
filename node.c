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

#include "msg.h"
#include "node.h"

// The purpose of this file is to provide insight into how to make various
// library calls to perfom some of the key functions required by the
// application. You are free to completely ignore anything in this file and do
// things whatever way you want provided it conforms with the assignment
// specifications. Note: this file compiles and works on the deparment servers
// running Linux. If you work in a different environment your mileage may vary.
// Remember that whatever you do the final program must run on the department
// Linux machines.

void usage(char *cmd) {
  printf(
      "usage: %s  portNum groupFileList logFile timeoutValue averageAYATime "
      "failureProbability \n",
      cmd);
}

// Global variables
int coordIndex = -1;  // The index for the current coordinator node in the group
int nodeIndex = -1;   // The index of the current node in the group, used to
                      // check for coordIndex
node nodes[MAX_NODES];  // array of nodes (structs) in the group
int numNodes = 0;       // the actual # of nodes in the groupListFile (0-8)
int electID = 1;
struct clock vectorClock[MAX_NODES];
FILE *logFile;  // file pointer to the node's log file
unsigned long sendFailureProbability;

int main(int argc, char **argv) {
  unsigned long port;
  char *groupListFileName;
  char *logFileName;
  unsigned long timeoutValue;
  unsigned long AYATime;

  // Parsing command line arguments
  if (argc != 7) {
    usage(argv[0]);
    return -1;
  }

  char *end;
  int err = 0;
  int fErr = 0;

  port = strtoul(argv[1], &end, 10);
  if (argv[1] == end) {
    printf("Port conversion error\n");
    err++;
  }

  // Parse all the host and portnos from groupListFile into node structs
  groupListFileName = argv[2];

  // Case where groupListFileName = "-", manually enter
  if (strcmp(groupListFileName, "-") == 0) {
    while (!feof(stdin)) {
      char inhost[100];
      int inport;

      scanf("%s %d", inhost, &inport);
      if (numNodes < MAX_NODES && inport != 0) {
        char *hostName = calloc(strlen(inhost) + 1, sizeof(char));
        strcpy(hostName, inhost);
        node newNode = {hostName, (unsigned long)inport};
        nodes[numNodes] = newNode;
        numNodes++;
      }
      memset(inhost, 0, sizeof(inhost));  // reset inhost buffer after newline
      inport = 0;
    }
  } else {
    // Case parse specified group mgmt file into nodes data struct
    FILE *groupFile;
    char str[1000];

    groupFile = fopen(groupListFileName, "r");
    if (groupFile == NULL) {
      printf("Could not open file %s \n", groupListFileName);
      fErr++;
    } else {
      while (fgets(str, 1000, groupFile) != NULL && numNodes < MAX_NODES) {
        strtok(str, " ");
        char *hostName = calloc(strlen(str) + 1, sizeof(char));
        strcpy(hostName, str);
        unsigned long portNo = atoi(strtok(NULL, " "));
        // create a new node and add it to list of nodes (max 9)
        node newNode = {hostName, portNo};
        nodes[numNodes] = newNode;
        numNodes++;
      }
      fclose(groupFile);
    }
  }

  logFileName = argv[3];
  logFile = fopen(logFileName, "w");  // file ptr to the log file to be written

  if (logFile == NULL) {
    printf("cannot open log file to write, program exiting.\n");
    return -1;
  }

  timeoutValue = strtoul(argv[4], &end, 10);
  if (argv[4] == end) {
    printf("Timeout value conversion error\n");
    err++;
  }

  AYATime = strtoul(argv[5], &end, 10);
  if (argv[5] == end) {
    printf("AYATime conversion error\n");
    err++;
  }

  sendFailureProbability = strtoul(argv[6], &end, 10);
  if (argv[6] == end) {
    printf("sendFailureProbability conversion error\n");
    err++;
  }

  if (sendFailureProbability < 0 || sendFailureProbability > 99) {
    printf("Invalid input sendFailureProbability should be between [0,99]\n");
    err++;
  }

  printf("Port number:              %d\n", port);
  printf("Group list file name:     %s\n", groupListFileName);
  printf("Log file name:            %s\n", logFileName);
  printf("Timeout value:            %d\n", timeoutValue);
  printf("AYATime:                  %d\n", AYATime);
  printf("Send failure probability: %d\n", sendFailureProbability);

  if (err) {
    printf("%d conversion error%sencountered, program exiting.\n", err,
           err > 1 ? "s were " : " was ");
    return -1;
  }

  if (fErr) {
    printf("file error encountered, program exiting.\n");
    return -1;
  }

  // Check if the newly created node is listed in the group mgmt file
  int pErr = 1;

  for (int i = 0; i < numNodes; i++) {
    if (nodes[i].portNo == port) {
      nodeIndex = i;  // set nodeIndex to its proper value
      pErr = 0;
    }
  }

  if (pErr) {
    printf("port # not found in group management file, program exiting.\n");
    return -1;
  }

  // This is some sample code to setup a UDP socket for sending and receiving.
  int sockfd;
  struct sockaddr_in servAddr;

  // Create the socket
  // The following must be one of the parameters don't leave this as it is
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket creation failed");
    exit(EXIT_FAILURE);
  }

  // set timeout for socket
  struct timeval timeout = {timeoutValue, 0};
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                 sizeof(timeout)) < 0) {
    perror("set socket timeout failed");
    exit(EXIT_FAILURE);
  }

  // Setup my server information
  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_port = htons(port);
  // Accept on any of the machine's IP addresses.
  servAddr.sin_addr.s_addr = INADDR_ANY;

  // Bind the socket to the requested addresses and port
  if (bind(sockfd, (const struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  // At this point the socket is setup and can be used for both
  // sending and receiving

  // Resolve hostnames to ip addr and store in nodes data struct for security
  // check later
  for (int i = 0; i < numNodes; i++) {
    struct addrinfo hints, *serverAddr;
    serverAddr = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AF_INET;
    hints.ai_protocol = IPPROTO_UDP;

    char p[sizeof(unsigned int)];
    sprintf(p, "%d", port);

    if (getaddrinfo(nodes[i].hostName, p, &hints, &serverAddr)) {
      printf("Couldn't lookup hostname\n");
      return -1;
    }

    // convert all hostnames and ip addr in group file to actual hostnames for
    // easier security check later
    char recvHostname[1025];
    int err = getnameinfo((struct sockaddr *)(serverAddr->ai_addr),
                          (socklen_t)(serverAddr->ai_addrlen), recvHostname,
                          sizeof(recvHostname), 0, 0, 0);
    if (err != 0) {
      printf("ERROR getnameinfo in parsing\n");
    }

    char *temp = calloc(strlen(recvHostname) + 1, sizeof(char));
    strcpy(temp, recvHostname);
    nodes[i].hostName = temp;
  }

  // Log starting Node # and instantiate vectorClock for all 9 nodes
  struct clock tempClock = {0, 0};
  for (int i = 0; i < MAX_NODES; i++) {
    if (i < numNodes) {
      struct clock newClock = {nodes[i].portNo, 0};
      vectorClock[i] = newClock;
    } else {
      // unused nodes has id = 0
      vectorClock[i] = tempClock;
    }
  }

  vectorClock[nodeIndex].time++;  // increment time by 1 before starting node
  fprintf(logFile, "Starting N%d\n", port);
  logClock(port);

  // Simplest case where there is only one node in group file
  if (numNodes == 1) {
    coordIndex = nodeIndex;
    vectorClock[nodeIndex].time++;
    fprintf(logFile, "Declare self Coordinator\n");
    logClock(port);
  } else {
    // All other cases
    // First step: start an election and increment node's vector clock
    startElection(sockfd, port);

    // 2nd step: check node state (eg. coord or regular)
    if (coordIndex < 0) {
      printf("Election bug, no coordinator found, program exiting.\n");
      return -1;
    }
  }
  printf("coordindex is %d\n", coordIndex);

  // set up poll structs to check for messages waiting at socket
  struct pollfd fds[1];
  memset(fds, 0, sizeof(fds));
  fds[0].fd = sockfd;
  fds[0].events = POLLIN;

  for (;;) {
    // Case where node is Coordinator
    while (coordIndex == nodeIndex) {
      int num_events = poll(fds, 1, -1);
      if (num_events == -1) {
        printf("polling error as coord\n");
      }
      /*else if(num_events == 0){
        coord timeout case, call another election to prevent deadlock
        where every node in group thinks it is coordinator
        commented out for now due to
      https://piazza.com/class/k4z1jwz5p307lt?cid=231 startElection(sockfd,
      port);
      }  */
      else {
        // check for events on fd
        if (fds[0].revents & POLLIN) {
          // There's a message waiting for the coordinator, call helper
          printf("coordinator got a msg\n");
          coordRespond(sockfd, port);
        }
      }
    }

    // Continuously monitor Coordinator
    while (coordIndex != nodeIndex) {
      int num_events = poll(fds, 1, (AYATime * 1000));
      if (num_events == -1) {
        // error case
        printf("polling error as node\n");
      } else if (num_events == 0) {
        // timeout case, send AYA
        vectorClock[nodeIndex].time++;
        fprintf(logFile, "Sending AYA msg to N%d\n", nodes[coordIndex].portNo);
        logClock(port);
        struct msg ayaMsg = createMsg(AYA, port);
        sendMsg(sockfd, ayaMsg, nodes[coordIndex].hostName,
                nodes[coordIndex].portNo);
        // wait for IAA, block on recv
        waitAYAResult(sockfd, port);
      } else {
        // check for events on fd
        if (fds[0].revents & POLLIN) {
          // There's a message waiting for the coordinator, call helper
          printf("node got a msg\n");
          nodeRespond(sockfd, port);
        }
      }
    }
  }
}

void nodeRespond(int sockfd, unsigned long port) {
  // Initialize recv
  struct sockaddr_in client;
  int len = sizeof(client);
  struct msg recvMsg;
  int n;
  memset(&client, 0, sizeof(client));
  n = recvfrom(sockfd, &recvMsg, sizeof(struct msg), MSG_WAITALL,
               (struct sockaddr *)&client, (socklen_t *)&len);

  // First check if msg source is from nodes within group, discard o.w
  // resovle msg source's ip addr into hostname for validitiy check
  unsigned long recvPort = (unsigned long)ntohs(client.sin_port);
  char recvHostname[1025];
  int err = getnameinfo((struct sockaddr *)&client, (socklen_t)len,
                        recvHostname, sizeof(recvHostname), 0, 0, 0);
  if (err != 0) {
    printf("ERROR getnameinfo \n");
  }
  // printf("got msg from host: %s port:%d \n", recvHostname, recvPort);

  if (isValidSource(recvHostname, recvPort)) {
    // convert all msg fields back host byteorder after recv
    recvMsg.msgID = ntohl(recvMsg.msgID);

    for (int i = 0; i < MAX_NODES; i++) {
      // vector clock parsing and synchronization
      recvMsg.vectorClock[i].nodeId = ntohl(recvMsg.vectorClock[i].nodeId);
      recvMsg.vectorClock[i].time = ntohl(recvMsg.vectorClock[i].time);
    }

    // Sync clocks and update local clock by 1
    syncVectorClock(recvMsg.vectorClock);
    vectorClock[nodeIndex].time++;

    switch (recvMsg.msgID) {
      case ELECT:
        fprintf(logFile, "Receive Election MSG from N%d\n", recvPort);
        logClock(port);
        // Reply with Answer to sender
        vectorClock[nodeIndex].time++;
        fprintf(logFile, "Send Answer to N%d\n", recvPort);
        logClock(port);
        struct msg ansMsg = createMsg(ANSWER, electID);
        sendMsg(sockfd, ansMsg, recvHostname, recvPort);
        // Call new election
        electID++;
        startElection(sockfd, port);
        break;
      case COORD:
        // Update coordIndex and change to monitoring state
        fprintf(logFile, "Receive COORD from N%d\n", recvPort);
        logClock(port);
        for (int i = 0; i < numNodes; i++) {
          if (nodes[i].portNo == recvPort && port < recvPort) {
            coordIndex = i;
          }
        }
        break;
      default:
        fprintf(logFile, "Discarding IAA/AYA/Answer from N%d\n", recvPort);
        logClock(port);
        break;
    }
  } else {
    // ignore msg since its sender is not part of the group, keep on recving
    fprintf(logFile, "Invalid message received, discarding\n");
    vectorClock[nodeIndex].time++;
    logClock(port);
  }
}

void coordRespond(int sockfd, unsigned long port) {
  // Initialize recv
  struct sockaddr_in client;
  int len = sizeof(client);
  struct msg recvMsg;
  int n;
  memset(&client, 0, sizeof(client));
  n = recvfrom(sockfd, &recvMsg, sizeof(struct msg), MSG_WAITALL,
               (struct sockaddr *)&client, (socklen_t *)&len);

  // First check if msg source is from nodes within group, discard o.w
  // resovle msg source's ip addr into hostname for validitiy check
  unsigned long recvPort = (unsigned long)ntohs(client.sin_port);
  char recvHostname[1025];
  int err = getnameinfo((struct sockaddr *)&client, (socklen_t)len,
                        recvHostname, sizeof(recvHostname), 0, 0, 0);
  if (err != 0) {
    printf("ERROR getnameinfo \n");
  }
  // printf("got msg from host: %s port:%d \n", recvHostname, recvPort);

  if (isValidSource(recvHostname, recvPort)) {
    // convert all msg fields back host byteorder after recv
    recvMsg.msgID = ntohl(recvMsg.msgID);

    for (int i = 0; i < MAX_NODES; i++) {
      // vector clock parsing and synchronization
      recvMsg.vectorClock[i].nodeId = ntohl(recvMsg.vectorClock[i].nodeId);
      recvMsg.vectorClock[i].time = ntohl(recvMsg.vectorClock[i].time);
    }

    // Sync clocks and update local clock by 1
    syncVectorClock(recvMsg.vectorClock);
    vectorClock[nodeIndex].time++;

    switch (recvMsg.msgID) {
      case ELECT:
        fprintf(logFile, "Receive Election MSG from N%d\n", recvPort);
        logClock(port);
        // Reply with Answer to sender
        vectorClock[nodeIndex].time++;
        fprintf(logFile, "Send Answer to N%d\n", recvPort);
        logClock(port);
        struct msg ansMsg = createMsg(ANSWER, electID);
        sendMsg(sockfd, ansMsg, recvHostname, recvPort);
        // Call new election
        electID++;
        startElection(sockfd, port);
        break;
      case COORD:
        // Update coordIndex and change to monitoring state
        fprintf(logFile, "Receive COORD from N%d\n", recvPort);
        logClock(port);
        for (int i = 0; i < numNodes; i++) {
          // Update coord only if
          if (nodes[i].portNo == recvPort && port < recvPort) {
            coordIndex = i;
          }
        }
        break;
      case AYA:
        fprintf(logFile, "Receive AYA MSG from N%d\n", recvPort);
        logClock(port);
        // Respond with IAA msg to sender
        vectorClock[nodeIndex].time++;
        fprintf(logFile, "Send IAA to N%d\n", recvPort);
        logClock(port);
        struct msg iaaMsg = createMsg(IAA, port);
        sendMsg(sockfd, iaaMsg, recvHostname, recvPort);
        break;
      default:
        fprintf(logFile, "Discarding IAA/Answer from N%d\n", recvPort);
        logClock(port);
        break;
    }
  } else {
    // ignore msg since its sender is not part of the group, keep on recving
    fprintf(logFile, "Invalid message received, discarding\n");
    vectorClock[nodeIndex].time++;
    logClock(port);
  }
}

/* Helper that calls/start an election */
void startElection(int sockfd, unsigned long port) {
  for (int i = 0; i < numNodes; i++) {
    // send elect msg to all nodes with higher #
    if (nodes[i].portNo > port) {
      vectorClock[nodeIndex].time++;
      struct msg electMsg = createMsg(ELECT, electID);
      fprintf(logFile, "Send Election MSG to N%d\n", nodes[i].portNo);
      logClock(port);
      if (sendMsg(sockfd, electMsg, nodes[i].hostName, nodes[i].portNo) < 0) {
        printf("Initial election msg failed to send, program exiting.\n");
      }
    }
  }
  waitElectionResult(sockfd, port, 0, 0);
}

/* Helper to wait and parse IAA response */
void waitAYAResult(int sockfd, unsigned long port) {
  // Initialize recv
  struct sockaddr_in client;
  int len = sizeof(client);
  struct msg recvMsg;
  int n;
  memset(&client, 0, sizeof(client));
  n = recvfrom(sockfd, &recvMsg, sizeof(struct msg), MSG_WAITALL,
               (struct sockaddr *)&client, (socklen_t *)&len);

  // AYA msg timed out, Coordinator failed, call new election
  if (n < 0) {
    // set coordIndex to unknown
    coordIndex = -1;
    printf("detected coord failed, call new elect\n");
    startElection(sockfd, port);
  } else {
    // First check if msg source is from nodes within group, discard o.w
    // resovle msg source's ip addr into hostname for validitiy check
    unsigned long recvPort = (unsigned long)ntohs(client.sin_port);
    char recvHostname[1025];
    int err = getnameinfo((struct sockaddr *)&client, (socklen_t)len,
                          recvHostname, sizeof(recvHostname), 0, 0, 0);
    if (err != 0) {
      printf("ERROR getnameinfo \n");
    }
    // printf("got msg from host: %s port:%d \n", recvHostname, recvPort);

    if (isValidSource(recvHostname, recvPort)) {
      // convert all msg fields back host byteorder after recv
      recvMsg.msgID = ntohl(recvMsg.msgID);

      for (int i = 0; i < MAX_NODES; i++) {
        // vector clock parsing and synchronization
        recvMsg.vectorClock[i].nodeId = ntohl(recvMsg.vectorClock[i].nodeId);
        recvMsg.vectorClock[i].time = ntohl(recvMsg.vectorClock[i].time);
      }

      // Sync clocks and update local clock by 1
      syncVectorClock(recvMsg.vectorClock);
      vectorClock[nodeIndex].time++;

      switch (recvMsg.msgID) {
        case ELECT:
          fprintf(logFile, "Receive Election MSG from N%d\n", recvPort);
          logClock(port);
          // reply ANSWER and forward election
          vectorClock[nodeIndex].time++;
          fprintf(logFile, "Send Answer to N%d\n", recvPort);
          logClock(port);
          struct msg ansMsg = createMsg(ANSWER, electID);
          sendMsg(sockfd, ansMsg, recvHostname, recvPort);
          startElection(sockfd, port);
          break;
        case COORD:
          // Update coordIndex and change to monitoring state
          fprintf(logFile, "Receive COORD from N%d\n", recvPort);
          logClock(port);
          for (int i = 0; i < numNodes; i++) {
            // Update coord only if it's larger than you
            if (nodes[i].portNo == recvPort && port < recvPort) {
              coordIndex = i;
              break;
            }
          }
          break;
        case IAA:
          fprintf(logFile, "Receive IAA from N%d\n", recvPort);
          logClock(port);
          if (nodes[coordIndex].portNo != recvPort) {
            waitAYAResult(sockfd, port);
          }
          break;
        default:
          fprintf(logFile, "Discarding AYA/Answer msg from N%d\n", recvPort);
          logClock(port);
          waitAYAResult(sockfd, port);
          break;
      }
    } else {
      // ignore msg since its sender is not part of the group, keep on recving
      // note known bug: if keep recving msgs but not IAA, won't timeout
      fprintf(logFile, "Invalid message received, discarding\n");
      vectorClock[nodeIndex].time++;
      logClock(port);
      waitAYAResult(sockfd, port);
    }
  }
}

/* Helper to wait and parse election responses */
void waitElectionResult(int sockfd, unsigned long port, int loopIter,
                        int gotAns) {
  // Check if COORD message isn't received within ((MAX_NODES + 1) timeout value
  if (loopIter > (MAX_NODES + 1)) {
    // abandon old election, start new election
    fprintf(logFile, "No COORD recv after ANS, abandonning old Election%d\n",
            electID);
    vectorClock[nodeIndex].time++;
    logClock(port);
    electID++;
    startElection(sockfd, port);
    return;
  }
  // Initialize recv
  struct sockaddr_in client;
  int len = sizeof(client);
  struct msg recvMsg;
  int n;
  memset(&client, 0, sizeof(client));
  n = recvfrom(sockfd, &recvMsg, sizeof(struct msg), MSG_WAITALL,
               (struct sockaddr *)&client, (socklen_t *)&len);

  // Elect msg timed out, node is new coordinator case
  if (n < 0) {
    if (gotAns) {
      waitElectionResult(sockfd, port, ++loopIter, gotAns);
      return;
    }
    // declare self as coord, increment own clock and log event
    coordIndex = nodeIndex;
    vectorClock[nodeIndex].time++;
    fprintf(logFile, "Declare self Coordinator\n");
    logClock(port);

    // send coord msg to all nodes with lower port #
    for (int i = 0; i < numNodes; i++) {
      if (nodes[i].portNo < port) {
        vectorClock[nodeIndex].time++;
        struct msg coordMsg = createMsg(COORD, electID);
        fprintf(logFile, "Sending coord to N%d\n", nodes[i].portNo);
        logClock(port);
        sendMsg(sockfd, coordMsg, nodes[i].hostName, nodes[i].portNo);
      }
    }
  } else {
    // First check if msg source is from nodes within group, discard o.w
    // resovle msg source's ip addr into hostname for validitiy check
    unsigned long recvPort = (unsigned long)ntohs(client.sin_port);
    char recvHostname[1025];
    int err = getnameinfo((struct sockaddr *)&client, (socklen_t)len,
                          recvHostname, sizeof(recvHostname), 0, 0, 0);
    if (err != 0) {
      printf("ERROR getnameinfo \n");
    }
    // printf("got msg from host: %s port:%d \n", recvHostname, recvPort);

    if (isValidSource(recvHostname, recvPort)) {
      // convert all msg fields back host byteorder after recv
      recvMsg.msgID = ntohl(recvMsg.msgID);

      for (int i = 0; i < MAX_NODES; i++) {
        // vector clock parsing and synchronization
        recvMsg.vectorClock[i].nodeId = ntohl(recvMsg.vectorClock[i].nodeId);
        recvMsg.vectorClock[i].time = ntohl(recvMsg.vectorClock[i].time);
      }

      // Sync clocks and update local clock by 1
      syncVectorClock(recvMsg.vectorClock);
      vectorClock[nodeIndex].time++;

      switch (recvMsg.msgID) {
        case ELECT:
          fprintf(logFile, "Receive Election MSG from N%d\n", recvPort);
          logClock(port);
          // Initial election, no need to forward election, just reply ANSWER
          vectorClock[nodeIndex].time++;
          fprintf(logFile, "Send Answer to N%d\n", recvPort);
          logClock(port);
          struct msg ansMsg = createMsg(ANSWER, electID);
          sendMsg(sockfd, ansMsg, recvHostname, recvPort);
          if (gotAns == 0) {
            waitElectionResult(sockfd, port, loopIter, gotAns);
          } else {
            waitElectionResult(sockfd, port, ++loopIter, gotAns);
          }
          break;
        case ANSWER:
          fprintf(logFile, "Receive Answer from N%d\n", recvPort);
          logClock(port);
          waitElectionResult(sockfd, port, ++loopIter, 1);
          break;
        case COORD:
          // Update coordIndex and change to monitoring state
          fprintf(logFile, "Receive COORD from N%d\n", recvPort);
          logClock(port);
          for (int i = 0; i < numNodes; i++) {
            // Update coord only if
            if (nodes[i].portNo == recvPort && port < recvPort) {
              coordIndex = i;
              break;
            }
          }
          break;
        default:
          fprintf(logFile, "Discarding IAA/AYA from N%d\n", recvPort);
          logClock(port);
          if (gotAns == 0) {
            waitElectionResult(sockfd, port, loopIter, gotAns);
          } else {
            waitElectionResult(sockfd, port, ++loopIter, gotAns);
          }
          break;
      }
    } else {
      // ignore msg since its sender is not part of the group, keep on recving
      fprintf(logFile, "Invalid message received, discarding\n");
      vectorClock[nodeIndex].time++;
      logClock(port);
      if (gotAns == 0) {
        waitElectionResult(sockfd, port, loopIter, gotAns);
      } else {
        waitElectionResult(sockfd, port, ++loopIter, gotAns);
      }
    }
  }
}

// Helper to create msg to be sent
struct msg createMsg(msgType type, int electId) {
  struct clock vectorClockCopy[MAX_NODES];
  memcpy(&vectorClockCopy, &vectorClock, sizeof(struct clock) * MAX_NODES);
  struct clock vectTemp[MAX_NODES];
  struct msg msg = {type, electId, *vectTemp};
  memcpy(&msg.vectorClock, &vectorClockCopy, sizeof(struct clock) * MAX_NODES);
  return msg;
}

// helper to send UDP datagrams, return -1 on error
int sendMsg(int sockfd, struct msg msg, char *dstHost, unsigned long dstPort) {
  // This would normally be a real hostname or IP address
  // as opposed to localhost
  struct addrinfo hints, *serverAddr;
  serverAddr = NULL;

  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_family = AF_INET;
  hints.ai_protocol = IPPROTO_UDP;

  char p[sizeof(unsigned int)];
  sprintf(p, "%d", dstPort);

  if (getaddrinfo(dstHost, p, &hints, &serverAddr)) {
    printf("Couldn't lookup hostname\n");
    return -1;
  }

  int bytesSent;

  // convert all msg fields in datagram to network byteorder before send
  msg.msgID = htonl(msg.msgID);
  msg.electionID = htonl(msg.electionID);
  for (int i = 0; i < MAX_NODES; i++) {
    msg.vectorClock[i].nodeId = htonl(msg.vectorClock[i].nodeId);
    msg.vectorClock[i].time = htonl(msg.vectorClock[i].time);
  }

  // Simulator network delivery failure by dropping packets based on RNG
  int random = (rand() % 100) + 1;

  if (sendFailureProbability < random) {
    printf("msg actually sent, not dropped\n");
    bytesSent = sendto(sockfd, &msg, sizeof(struct msg), MSG_CONFIRM,
                       serverAddr->ai_addr, serverAddr->ai_addrlen);
    if (bytesSent != sizeof(struct msg)) {
      perror("UDP send failed: ");
      return -1;
    }
  }
  freeaddrinfo(serverAddr);
  return 0;
}

/* Helper for logging vector clock to the log file */
void logClock(unsigned long port) {
  fflush(logFile);
  int isFirst = 1;
  fprintf(logFile, "N%d {", port);
  for (int i = 0; i < numNodes; i++) {
    if (vectorClock[i].time > 0) {
      if (!isFirst) {
        fprintf(logFile, ", ");
      }
      fprintf(logFile, "\"N%d\" : %d", vectorClock[i].nodeId,
              vectorClock[i].time);
      isFirst = 0;
    }
  }
  fprintf(logFile, "}\n");
  fflush(logFile);
}

// If hostname and port pair is found in group, return valid, else 0
int isValidSource(char *srcHost, unsigned long srcPort) {
  for (int i = 0; i < numNodes; i++) {
    if (strcmp(srcHost, nodes[i].hostName) == 0 && nodes[i].portNo == srcPort) {
      return 1;
    }
  }
  return 0;
}

void syncVectorClock(struct clock recvVectorClock[]) {
  // map the recvVectorClock's time in terms of local vecClock index
  int indexMap[MAX_NODES];
  // reorder the recvVectorClock to local vector clock's order
  for (int i = 0; i < MAX_NODES; i++) {
    for (int j = 0; j < MAX_NODES; j++) {
      if (vectorClock[i].nodeId == recvVectorClock[j].nodeId) {
        indexMap[i] = recvVectorClock[j].time;
      }
    }
  }
  // for (int i = 0; i < MAX_NODES; i++) {
  //   printf("indexmap: %d vs vectorClock: %d\n", indexMap[i],
  //          vectorClock[i].time);
  // }
  if (indexMap[nodeIndex] <= vectorClock[nodeIndex].time) {
    for (int i = 0; i < numNodes; i++) {
      if (i != nodeIndex) {
        vectorClock[i].time = (vectorClock[i].time > indexMap[i])
                                  ? vectorClock[i].time
                                  : indexMap[i];
      }
    }
  } else {
    printf("there's bug in code, clock not in sync or node restarted\n");
    exit(0);
  }
}