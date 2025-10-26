// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"

#include <string.h>
#include <stdio.h>

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

  if( r != 0){
    return;
  }

  if (connectionParameters.role == 0){

    FILE* file = fopen(filename, "rb");

    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    fseek(file, 0, SEEK_END); 

    size_t size = ftell(file);

    fseek(file, 0, SEEK_SET);

    printf("0x%lX", size);
    
    unsigned char cP[50] = {0};

    int c_size = buildCtrlPacket(cP, filename, size);


    llwrite(cP, c_size);

    unsigned char rbuf[1000] = {0};

    int rBytes = 0;
    
    while((rBytes = fread(rbuf, sizeof(unsigned char), 1000, file)) > 0){
      
      unsigned char dp[1100] = {0};
      dp[0] = 2;
      dp[1] = (rBytes >> 8) & 0xFF;
      dp[2] = rBytes & 0xFF;

      memcpy(&dp[3],rbuf, rBytes);

      llwrite(dp, rBytes + 3);
    }
    cP[0] = 3;

    llwrite(cP,5);
    
    fclose(file);

    llclose();
  }

  if(connectionParameters.role == 1){

    FILE* file = fopen(filename, "w");

    unsigned char cpacket[600] = {0};
    llread(cpacket);
    int STOP = 1;
    while(STOP){

      unsigned char packet[2000] = {0};

      int packetBytes = llread(packet);
      if(packetBytes > 0){
        printf("Received packet: ");

        int pSize = packet[1]*256 + packet[2]; 
        
        printf("Packet Size = %d ", pSize);

        for(int i = 0; i < packetBytes; i++){
            printf("0x%02X ", packet[i]);
        }
        if(packet[0] == 3){
          STOP = 0;
          break;
        }

        fwrite(&packet[3], sizeof(unsigned char),pSize, file);
      }
    }
      llclose();

  }
  return;
}




int buildCtrlPacket(unsigned char *cbuf, const char * filename, size_t filesize){

    int size_bytes = 0;
    int index = 0;
    size_t test = filesize;
    while (test > 0){
      size_bytes++;
      test = test >> 8;
    }

    cbuf[index++] = 1;
    cbuf[index++] = 0;
    cbuf[index++] = size_bytes;
    for(int i = 1; i <= size_bytes; i++){
      cbuf[index++] = (filesize >> (8 * (size_bytes - i))) & 0xFF;
    }




    cbuf[index++] = 1;
    int name_bytes = index;
    cbuf[index++] = 0;
    int i = 0;
    while (filename[i] != '\0'){
      cbuf[name_bytes] += 1;
      cbuf[index++] = filename[i];
      i++;
    }

    return index+1;
}


int readCtrlPacket(unsigned char * cbuf, const char * filename, size_t filesize){
  
}
