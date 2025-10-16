// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"

#include <string.h>

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename) {
  LinkLayer connectionParameters;
  connectionParameters.baudRate = baudRate;
  strcpy(connectionParameters.serialPort, serialPort);
  if (strcmp("tx", role) == 0) {
    connectionParameters.role = 0;
  } else {
    connectionParameters.role = 1;
  }
  connectionParameters.timeout = 3;
  connectionParameters.nRetransmissions = nTries;
  int r = llopen(connectionParameters);
    if (connectionParameters.role == 0){

    FILE* file = fopen("peguin.gif", "rb");

    if (file == NULL) {
        perror("Error opening file");
        return;
    }
    
    
    unsigned char cP[50] = {0};
    cP[0] = 1;
    cP[1] = 0;
    cP[2] = 2;
    cP[3] = 0x2A;
    cP[4] = 0xF8;


    llwrite(cP,5);

    unsigned char rbuf[500]
    
    while( int rBytes = fread(rbuf,sizeof(unsigned char), 500, file)){
      
      unsigned char dp[1000] = {0};
      dp[0] = 2;
      dp[1] = 1;
      dp[2] = 0xFD;
      memcpy(&dp[3],rbuf, rBytes);

      llwrite(dp, rBytes + 3);

    }
    cP[0] = 3;
    llwrite(cP,5);
    fclose(file);
  }

  if(connectionParameters.role == 1){
    unsigned char* packet;
    llread(packet);
  }
  llclose();
  return;
}
