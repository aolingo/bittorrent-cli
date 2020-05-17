/* */
#define MAX_NAME_LENGTH 255

typedef struct {
  char announce[MAX_NAME_LENGTH];  // tracker server address
  char outName[MAX_NAME_LENGTH];   // suggested name to save file
  int pieceLength;                 // size in bytes of each piece of file
  int length;                      // total length of the file to be downloaded
  int numPieces;                   // total number of pieces
  char**
      pieces;  // string consisting of the concatenation of all 20-byte SHA1
               // hash values, one per piece (byte string, i.e. not urlencoded)
} torrentInfo;

// info: A dictionary with the following keys–name: The suggested name for
// saving the file–piece length: The size, in bytes, of a piece for this file.
// This is always a power of 2.–length: The length of the file to be
// downloaded–pieces:  A string of length in multiples of 20 bytes.  Each 20
// bytes subpart represents theSHA1 hash of the piece of the file at that index.

/* Helpers for bitclient file */

// Helper to check validity of cli arguments
int isArgValid(char* torrentPath, char* outPath);