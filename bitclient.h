/* Helpers for bitclient file */

typedef struct {
  char announce[255];  // tracker server address
  char outName[255];   // suggested name to save file
  int pieceLength;     // number of bytes in each piece of file
};

// Helper to check validity of cli arguments
int isArgValid(char* torrentPath, char* outPath);