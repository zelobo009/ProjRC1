// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"

#include <string.h>
#include <stdio.h>

int readCtrlPacket(unsigned char *cbuf, char *rfilename, size_t *rfilesize);

int buildCtrlPacket(unsigned char *cbuf, const char *filename, size_t filesize);

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
  LinkLayer connectionParameters;
  connectionParameters.baudRate = baudRate;
  strcpy(connectionParameters.serialPort, serialPort);
  if (strcmp("tx", role) == 0)
  {
    connectionParameters.role = 0;
  }
  else
  {
    connectionParameters.role = 1;
  }
  connectionParameters.timeout = 3;
  connectionParameters.nRetransmissions = nTries;
  int r = llopen(connectionParameters);

  if (r != 0)
  {
    return;
  }

  if (connectionParameters.role == 0)
  {

    FILE *file = fopen(filename, "rb");

    if (file == NULL)
    {
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
    int timeout = 0;

    while ((rBytes = fread(rbuf, sizeof(unsigned char), 1000, file)) > 0)
    {

      unsigned char dp[1100] = {0};
      dp[0] = 2;
      dp[1] = (rBytes >> 8) & 0xFF;
      dp[2] = rBytes & 0xFF;

      memcpy(&dp[3], rbuf, rBytes);

      if (llwrite(dp, rBytes + 3) < 0)
      {
        timeout = 1;
        printf("Timeout.");
        break;
      }
    }
    cP[0] = 3;

    if (timeout)
    {
      printf("\nLost connection\n");
      return;
    }

    llwrite(cP, 5);

    fclose(file);

    llclose();
  }

  if (connectionParameters.role == 1)
  {

    unsigned char cpacket[600] = {0};
    llread(cpacket);

    char rfilename[256] = {0};
    size_t rfilesize = 0;
    int ctrl = readCtrlPacket(cpacket, rfilename, &rfilesize);
    if (ctrl != 1)
    {
      printf("Error reading intial control packet\n");
      return;
    }

    FILE *file = fopen(filename, "w");

    printf("Copying file %s to %s \n", rfilename, filename);

    if (file == NULL)
    {
      perror("Error opening file");
      return;
    }

    int STOP = 1;
    while (STOP)
    {

      unsigned char packet[2000] = {0};

      int packetBytes = llread(packet);
      if (packetBytes > 0)
      {
        printf("Received packet with ");

        int pSize = packet[1] * 256 + packet[2];

        printf("Packet Size = %d \n", pSize);

        if (packet[0] == 3)
        {
          STOP = 0;
          break;
        }

        fwrite(&packet[3], sizeof(unsigned char), pSize, file);
      }
    }
    llclose();
  }
  return;
}

int buildCtrlPacket(unsigned char *cbuf, const char *filename, size_t filesize)
{

  int size_bytes = 0;
  int index = 0;
  size_t test = filesize;
  while (test > 0)
  {
    size_bytes++;
    test = test >> 8;
  }

  cbuf[index++] = 1;
  cbuf[index++] = 0;
  cbuf[index++] = size_bytes;
  for (int i = 1; i <= size_bytes; i++)
  {
    cbuf[index++] = (filesize >> (8 * (size_bytes - i))) & 0xFF;
  }

  cbuf[index++] = 1;
  int name_bytes = index;
  cbuf[index++] = 0;
  int i = 0;
  while (filename[i] != '\0')
  {
    cbuf[name_bytes] += 1;
    cbuf[index++] = filename[i];
    i++;
  }

  return index + 1;
}

int readCtrlPacket(unsigned char *cbuf, char *rfilename, size_t *rfilesize)
{
  int index = 1;

  unsigned int control = cbuf[0];

  unsigned int T1 = cbuf[index++];
  unsigned int L1 = cbuf[index++];

  if (T1 == 0)
  {
    size_t size = 0;

    for (int i = 0; i < L1; i++)
      size = (size << 8) | cbuf[index++];

    *rfilesize = size;
  }
  unsigned int T2 = cbuf[index++];
  unsigned int L2 = cbuf[index++];
  if (T2 == 1)
  {

    for (int i = 0; i < L2; i++)
      rfilename[i] = cbuf[index++];

    rfilename[L2] = '\0';
  }

  return control;
}
