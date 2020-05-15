// Struct for parsing nodes from groupListFile
typedef struct node {
  char* hostName;
  unsigned long portNo;
} node;

// Helper to send UDP messages, return -1 on error
int sendMsg(int sockfd, struct msg msg, char* dstHost, unsigned long dstPort);

struct msg createMsg(msgType type, int electId);

void logClock(unsigned long port);

// Helper to check if datagram source has valid hostname and port (eg. within
// group)
int isValidSource(char* srcHost, unsigned long srcPort);

void startElection(int sockfd, unsigned long port);

void waitElectionResult(int sockfd, unsigned long port, int loopIter,
                        int gotAns);

void waitAYAResult(int sockfd, unsigned long port);

/* Helper to sync the recvVectorClock with the local node's vectorClock */
void syncVectorClock(struct clock recvVectorClock[]);

/* Helper for Coordinator node to parse and respond to msgs */
void coordRespond(int sockfd, unsigned long port);

/* Helper for regular node to parse and respond to msgs */
void nodeRespond(int sockfd, unsigned long port);