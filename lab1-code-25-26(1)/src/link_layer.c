// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
LinkLayer info;
void alarmHandler(int signal);

int check_Flag(unsigned char *buf, unsigned char add, unsigned char ctrl);

int changeCurrentPacket(unsigned char *CurrentPacket, unsigned char byte);

void receiveFlag(unsigned char *bufR, State *state, unsigned char byte);

int byte_destuffing(unsigned char *packet, unsigned char *result, int packetBytes, unsigned char *bcc2, int dataSize);

volatile int STOP = FALSE;
int alarmEnabled = FALSE;
int alarmCount = 0;
struct timespec start, end;
unsigned char CurrentPacket = 0x00;

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////

int llopen(LinkLayer connectionParameters)
{
  clock_gettime(CLOCK_MONOTONIC, &start);
  printf("Starting clock\n");
  info = connectionParameters;
  CurrentPacket = 0x00;
  alarmEnabled = FALSE;
  alarmCount = 0;
  const char *serialPort = connectionParameters.serialPort;
  State state = Start;

  if (openSerialPort(serialPort, connectionParameters.baudRate) < 0)
  {
    perror("openSerialPort");
    return -1;
  }

  printf("Serial port %s opened\n", serialPort);

  if (connectionParameters.role == 0)
  {

    struct sigaction act = {0};
    act.sa_handler = &alarmHandler;
    if (sigaction(SIGALRM, &act, NULL) == -1)
    {
      perror("sigaction");
      return 1;
    }

    printf("\nAlarm configured\n");

    // Create string to send
    unsigned char buf[1024] = {0};

    buf[0] = FLAG;
    buf[1] = add1;
    buf[2] = SET;
    buf[3] = add1 ^ SET;
    buf[4] = FLAG;

    while (alarmCount < connectionParameters.nRetransmissions &&
           !alarmEnabled)
    {
      int bytes = writeBytesSerialPort(buf, 5);
      printf("--SENDING SET-- \n");
      sleep(0.5);
      if (alarmEnabled == FALSE)
      {
        alarm(connectionParameters.timeout);
        alarmEnabled = TRUE;
      }

      STOP = FALSE;
      unsigned char bufR[BUF_SIZE] = {0};
      while (STOP == FALSE)
      {
        if (!alarmEnabled)
        {
          break;
        }
        unsigned char byte;
        readByteSerialPort(&byte);

        receiveFlag(bufR, &state, byte);
        if (state == Stop)
        {
          STOP = check_Flag(bufR, add1, UA);
          state = Start;
        }
      }
      if (alarmEnabled)
      {
        alarmCount = 0;
        alarm(0);

        printf("--RECEIVED UA-- \n");
        break;
      }
    }
    if (alarmCount >= connectionParameters.nRetransmissions)
    {
      printf("Failed to connect: no response from receiver\n");
      return -1;
    }

    return 0;
  }
  else
  {

    State state = Start;
    unsigned char bufR[6] = {0};
    while (STOP == FALSE)
    {

      unsigned char byte;
      int bytes = readByteSerialPort(&byte);

      receiveFlag(bufR, &state, byte);

      if (state == Stop)
      {
        STOP = check_Flag(bufR, add1, SET);
        state = Start;
      }
    }
    printf("--RECEIVED SET \n");
    printf("--SENDING UA-- \n");
    unsigned char buf[BUF_SIZE] = {0};

    buf[0] = FLAG;
    buf[1] = add1;
    buf[2] = UA;
    buf[3] = add1 ^ UA;
    buf[4] = FLAG;

    sleep(0.5);
    writeBytesSerialPort(buf, 5);
    printf("\n");


    return 0;
  }
}

void alarmHandler(int signal)
{
  alarmEnabled = FALSE;
  alarmCount++;
  printf("Alarm #%d received\n", alarmCount);
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////

int llwrite(const unsigned char *buf, int bufSize)
{
  alarmEnabled = FALSE;
  alarmCount = 0;
  int REJ = TRUE;
  State state = Start;

  unsigned char tbuf[bufSize * 2 + 6];
  memset(tbuf, 0, bufSize * 2 + 6);

  tbuf[0] = FLAG;
  tbuf[1] = add1;
  tbuf[2] = CurrentPacket;
  tbuf[3] = add1 ^ CurrentPacket;
  CurrentPacket = CurrentPacket ^ 0x80;

  unsigned char bcc2 = 0x00;
  int bytes_stuffed = stuff_packet(tbuf, buf, bufSize, &bcc2);

  tbuf[4 + bufSize + bytes_stuffed] = bcc2;
  tbuf[5 + bufSize + bytes_stuffed] = FLAG;

  printf("\n");

  struct sigaction act = {0};
  act.sa_handler = &alarmHandler;
  if (sigaction(SIGALRM, &act, NULL) == -1)
  {
    perror("sigaction");
    return 1;
  }
  int bytes = 0;
  printf("Alarm configured\n");
  REJ = TRUE;

  while ((alarmCount < info.nRetransmissions && !alarmEnabled) || REJ)
  {
    REJ = FALSE;
    bytes = writeBytesSerialPort(tbuf, bufSize + 6 + bytes_stuffed);
    printf("Sending Frame, Size =  %d \n", bufSize + 6 + bytes_stuffed);
    state = Start;
    printf("\n");

    sleep(0.3);
    if (alarmEnabled == FALSE)
    {
      alarm(info.timeout);
      alarmEnabled = TRUE;
    }

    STOP = FALSE;
    unsigned char bufR[BUF_SIZE] = {0};
    while (STOP == FALSE)
    {
      if (!alarmEnabled)
      {
        break;
      }
      unsigned char byte;
      readByteSerialPort(&byte);

      receiveFlag(bufR, &state, byte);
      if (state == Stop && changeCurrentPacket(&CurrentPacket, bufR[2]))
      {
        STOP = check_Flag(bufR, add1, bufR[2]);
        state = Start;
      }
    }

    if (alarmEnabled)
    {
      if (bufR[2] == 0x54 || bufR[2] == 0x55)
      {
        printf("--REJ%d RECEIVED--\n", bufR[2] == 0x55);
        printf("--RESENDING--\n ");
        REJ = TRUE;
        alarmCount = 0;
        alarm(info.timeout);
        printf("Alarm reseted\n");

      }
      else
      {
        printf("--RR%d RECEIVED--\n", bufR[2] == 0xAB);
        alarmCount = 0;
        alarm(0);
        break;
      }
    }
  }
  if (alarmCount >= info.nRetransmissions)
  {
    return -1;
  }
  return bytes;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{

  int nBytesBuf = 0;

  State state = Start;
  int packetBytes = 0;
  unsigned char bcc1 = 0;
  int dataSize = 0;
  int dup = 0;
  int wrong_bcc1 = 0;
  unsigned char bcc2 = 0x00;
  unsigned char result[2006];
  STOP = FALSE;

  while (STOP == FALSE)
  {

    unsigned char byte;
    int bytes = readByteSerialPort(&byte);
    nBytesBuf += bytes;
    if (state == Start && wrong_bcc1)
    {
      return -1;
    }
    switch (state)
    {
    case Start:
      if (byte == FLAG)
        state = Flag_RCV;

      else
        state = Start;

      break;

    case Flag_RCV:
      if (byte == FLAG)
        state = Flag_RCV;

      else if (byte == add1)
        state = A_RCV;

      else
        state = Start;

      break;

    case A_RCV:
      if (byte == FLAG)
        state = Flag_RCV;

      else if (byte == CurrentPacket)
      {
        CurrentPacket = CurrentPacket ^ 0x80;
        bcc1 = (byte ^ add1);
        state = BCC_DATA;
      }

      else if (byte == (CurrentPacket ^ 0x80))
      {
        state = BCC_DATA;
        bcc1 = (byte ^ add1);
        dup = 1;
      }

      else
      {
        state = Start;
      }
      break;

    case BCC_DATA:
      if (byte == FLAG)
        state = Flag_RCV;

      else if (byte == bcc1)
      {
        state = DATA_RCV;
      }
      else
      {
        wrong_bcc1 = 1;
        state = Start;
      }
      break;

    case DATA_RCV:

      if (byte == FLAG)
      {
        state = Stop;
        STOP = TRUE;
      }
      else
      {
        result[dataSize] = byte;
        dataSize++;
        state = DATA_RCV;
      }

      break;

    case Stop:
      STOP = TRUE;
      break;

    default:
      break;
    }
  }

  unsigned char test = 0x00;
  packetBytes = byte_destuffing(packet, result, packetBytes, &bcc2, dataSize);

  for (int i = 0; i < packetBytes; i++)
  {
    test = test ^ packet[i];
  }
  if (bcc2 == test)
  {
    unsigned char buf[5] = {0};
    buf[0] = FLAG;
    buf[1] = add1;
    buf[2] = 0xAA + (CurrentPacket == 0x80);
    buf[3] = add1 ^ buf[2];
    buf[4] = FLAG;
    int i = 0;
    printf("\n--Sending RR%d-- \n", (CurrentPacket == 0x80));
    sleep(0.3);
    writeBytesSerialPort(buf, 5);
  }
  else
  {
    printf("\n--REJECTED-- \n");
    CurrentPacket = (0x80 ^ CurrentPacket);
    unsigned char buf[5] = {0};
    buf[0] = FLAG;
    buf[1] = add1;
    buf[2] = 0x54 + (CurrentPacket == 0x80);
    buf[3] = add1 ^ buf[2];
    buf[4] = FLAG;

    printf("--Sending REJ%d-- \n", (CurrentPacket == 0x80));
    int i = 0;
    sleep(0.3);
    writeBytesSerialPort(buf, 5);
    return -1;
  }
  printf("\n");

  if (dup)
  {
    printf("--DUPLICATE PACKET--\n");
    printf("--DISCARDING--\n");
    return -1;
  }

  return packetBytes;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose()
{

  State state = Start;

  if (info.role == 0)
  {
    printf("\n Starting disconnect\n");
    alarmCount = 0;
    alarmEnabled = FALSE;
    struct sigaction act = {0};
    act.sa_handler = &alarmHandler;
    if (sigaction(SIGALRM, &act, NULL) == -1)
    {
      perror("sigaction");
      return 1;
    }

    printf("\nAlarm configured\n");

    // Create string to send
    unsigned char buf[1024] = {0};

    buf[0] = FLAG;
    buf[1] = add1;
    buf[2] = DISC;
    buf[3] = add1 ^ DISC;
    buf[4] = FLAG;

    while (alarmCount < info.nRetransmissions && !alarmEnabled)
    {
      writeBytesSerialPort(buf, 5);
      printf("--SENDING DISC-- \n");
      sleep(0.5);
      if (alarmEnabled == FALSE)
      {
        alarm(info.timeout);
        alarmEnabled = TRUE;
      }

      STOP = FALSE;
      unsigned char bufR[BUF_SIZE] = {0};
      while (STOP == FALSE)
      {
        if (!alarmEnabled)
        {
          break;
        }
        unsigned char byte;
        readByteSerialPort(&byte);

        receiveFlag(bufR, &state, byte);

        if (state == Stop)
        {
          STOP = check_Flag(bufR, add2, DISC);
          state = Start;
        }
      }
      if (alarmEnabled)
      {
        
        printf("\n");
        if (bufR[2] != DISC)
        {
          printf("Did not receive disconnect byte\n");
        }
        else
        {
          printf("--RECEIVED DISC-- \n");
          alarmCount = 0;
          alarm(0);
          break;
        }
      }
    }

    unsigned char bufS[5] = {0};

    bufS[0] = FLAG;
    bufS[1] = add2;
    bufS[2] = UA;
    bufS[3] = add2 ^ UA;
    bufS[4] = FLAG;
    sleep(0.5);
    writeBytesSerialPort(bufS, 5);

    printf("\n--SENDING UA-- \n");

    if (closeSerialPort() < 0)
    {
      perror("closeSerialPort");
    }
    printf("\nClosed the connection\n");
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("\nTotal time: %.6f seconds\n", elapsed);
    return 0;
  }
  else
  {
    int nBytesBuf = 0;

    State state = Start;
    STOP = FALSE;
    unsigned char bufR[BUF_SIZE] = {0};

    while (STOP == FALSE)
    {

      unsigned char byte;
      int bytes = readByteSerialPort(&byte);
      nBytesBuf += bytes;

      receiveFlag(bufR, &state, byte);

      if (state == Stop)
      {
        STOP = check_Flag(bufR, add1, DISC);
        state = Start;
      }
    }

    unsigned char buf[BUF_SIZE] = {0};

    printf("--RECEIVED DISC--\n");

    buf[0] = FLAG;
    buf[1] = add2;
    buf[2] = DISC;
    buf[3] = add2 ^ DISC;
    buf[4] = FLAG;

    state = Start;
    struct sigaction act = {0};
    act.sa_handler = &alarmHandler;
    if (sigaction(SIGALRM, &act, NULL) == -1)
    {
      perror("sigaction");
      return 1;
    }

    printf("\nAlarm configured\n");

    alarmCount = 0;
    alarmEnabled = FALSE;
    while (alarmCount < info.nRetransmissions && !alarmEnabled)
    {
      writeBytesSerialPort(buf, 5);
      printf("--SENDING DISC-- \n");
      printf("\n");
      if (alarmEnabled == FALSE)
      {
        alarm(info.timeout);
        alarmEnabled = TRUE;
      }

      STOP = FALSE;
      unsigned char bufR[BUF_SIZE] = {0};
      state = Start;
      while (STOP == FALSE)
      {
        if (!alarmEnabled)
        {
          break;
        }
        unsigned char byte;
        readByteSerialPort(&byte);

        receiveFlag(bufR, &state, byte);
        if (state == Stop)
        {
          STOP = check_Flag(bufR, add2, UA);
          state = Start;
        }
      }
      if (alarmEnabled)
      {
        printf("--RECEIVED UA-- \n");
        alarmCount = 0;
        alarm(0);
        break;
      }
    }
    if (closeSerialPort() < 0)
    {
      perror("closeSerialPort");
    }
    printf("\nClosed the connection\n");
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("\nTotal time: %.6f seconds\n", elapsed);
    return 0;
  }
}

int stuff_packet(unsigned char *stuffed_buf, const unsigned char *packet, int packetSize, unsigned char *bcc2)
{

  int bytes_stuffed = 0;

  for (int i = 0; i < packetSize; i++)
  {
    *bcc2 = *bcc2 ^ packet[i];
    if (packet[i] == 0x7E)
    {
      stuffed_buf[4 + i + bytes_stuffed] = 0x7D;
      stuffed_buf[4 + i + 1 + bytes_stuffed] = 0x5E;
      bytes_stuffed++;
    }
    else if (packet[i] == 0x7D)
    {
      stuffed_buf[4 + i + bytes_stuffed] = 0x7D;
      stuffed_buf[4 + i + 1 + bytes_stuffed] = 0x5D;
      bytes_stuffed++;
    }
    else
    {
      stuffed_buf[4 + i + bytes_stuffed] = packet[i];
    }
  }
  return bytes_stuffed;
}
int byte_destuffing(unsigned char *packet, unsigned char *result, int packetBytes, unsigned char *bcc2, int dataSize)
{

  *bcc2 = result[dataSize - 1];
  int i = 0;
  while (i < dataSize - 1)
  {
    if (result[i] == 0x7D)
    {
      if (result[i + 1] == 0x5d)
      {
        packet[packetBytes] = 0x7D;
        i++;
        packetBytes++;
      }
      else if (result[i + 1] == 0x5e)
      {
        packet[packetBytes] = 0x7E;
        i++;
        packetBytes++;
      }
    }
    else
    {
      packet[packetBytes] = result[i];
      packetBytes++;
    }
    i++;
  }
  return packetBytes;
}

int check_Flag(unsigned char *buf, unsigned char add, unsigned char ctrl)
{
  if (buf[1] == add && buf[2] == ctrl)
  {
    return 1;
  }
  return 0;
}

int changeCurrentPacket(unsigned char *CurrentPacket, unsigned char byte)
{
  if (byte == 0xAB || byte == 0xAA || byte == 0x55 || byte == 0x54)
  {
    return 1;
  }
  return 0;
}

void receiveFlag(unsigned char *bufR, State *state, unsigned char byte)
{

  switch (*state)
  {
  case Start:
    bufR[0] = '\0';
    if (byte == FLAG)
    {
      *state = Flag_RCV;
      bufR[0] = FLAG;
    }
    else
      *state = Start;

    break;

  case Flag_RCV:
    if (byte == FLAG)
      *state = Flag_RCV;

    else if (byte == add1 || byte == add2)
    {
      *state = A_RCV;
      bufR[1] = byte;
    }

    else
      *state = Start;

    break;

  case A_RCV:
    if (byte == FLAG)
      *state = Flag_RCV;

    else if (byte == UA || byte == SET || byte == DISC || byte == 0xAA || byte == 0xAB || byte == 0x55 || byte == 0x54)
    {
      *state = C_RCV;
      bufR[2] = byte;
    }

    else
      *state = Start;

    break;

  case C_RCV:
    if (byte == FLAG)
      *state = Flag_RCV;

    else if (byte == (bufR[2] ^ bufR[1]))
    {
      bufR[3] = (bufR[2] ^ bufR[1]);
      *state = BCC_RCV;
    }

    else
      *state = Start;

    break;

  case BCC_RCV:
    if (byte == FLAG)
    {
      bufR[4] = FLAG;
      *state = Stop;
    }

    else
      *state = Start;
    break;

  case Stop:
    break;
  default:
    break;
  }
  return;
}
