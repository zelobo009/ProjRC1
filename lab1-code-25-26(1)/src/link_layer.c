// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////

void alarmHandler(int signal);

volatile int STOP = FALSE;
int alarmEnabled = FALSE;
int alarmCount = 0;

int llopen(LinkLayer connectionParameters) {

  alarmEnabled = FALSE;
  alarmCount = 0;
  const char *serialPort = connectionParameters.serialPort;
  State state = Start;

  if (openSerialPort( serialPort, connectionParameters.baudRate) < 0) {
    perror("openSerialPort");
    return -1;
  }

  printf("Serial port %s opened\n", serialPort);

  if (connectionParameters.role == 0) {

    struct sigaction act = {0};
    act.sa_handler = &alarmHandler;
    if (sigaction(SIGALRM, &act, NULL) == -1) {
      perror("sigaction");
      return 1;
    }

    printf("Alarm configured\n");

    // Create string to send
    unsigned char buf[BUF_SIZE] = {0};

    buf[0] = FLAG;
    buf[1] = add1;
    buf[2] = SET;
    buf[3] = add1 ^ SET;
    buf[4] = FLAG;

    while (alarmCount < connectionParameters.nRetransmissions && !alarmEnabled) {
      int bytes = writeBytesSerialPort(buf, 5);
      printf("Sending control word: ");
      int i = 0;

      while (i < 5) {
        printf("0x%02X ", buf[i]);
        i++;
      }
      printf("%d bytes written to serial port\n", bytes);
      sleep(1);
      if (alarmEnabled == FALSE) {
        alarm(connectionParameters.timeout);
        alarmEnabled = TRUE;
      }

      STOP = FALSE;
      int lock = 0;
      int counter = 0;
      unsigned char bufR[BUF_SIZE] = {0};
      while (STOP == FALSE) {
        if (!alarmEnabled) {
          break;
        }
        unsigned char byte;
        int bytes = readByteSerialPort(&byte);

        switch (state) {
        case Start:
          printf("start\n");
          bufR[0] = '\0';
          if (byte == FLAG) {
            state = Flag_RCV;
            bufR[0] = FLAG;
          } else
            state = Start;

          break;

        case Flag_RCV:
          if (byte == FLAG)
            state = Flag_RCV;

          else if (byte == add1) {
            state = A_RCV;
            bufR[1] = add1;
          }

          else
            state = Start;

          break;

        case A_RCV:
          printf("a\n");
          if (byte == FLAG)
            state = Flag_RCV;

          else if (byte == UA) {
            state = C_RCV;
            bufR[2] = FLAG;
          }

          else
            state = Start;

          break;

        case C_RCV:
          printf("c\n");
          if (byte == FLAG)
            state = Flag_RCV;

          else if (byte == (UA ^ add1)) {
            bufR[3] = UA ^ add1;
            state = BCC_RCV;
          }

          else
            state = Start;

          break;

        case BCC_RCV:

          printf("bcc\n");
          if (byte == FLAG) {
            bufR[4] = FLAG;
            state = Stop;
            STOP = TRUE;
          }

          else
            state = Start;
          break;

        case Stop:
          printf("stop\n");
          STOP = TRUE;
          break;
        default:
          break;
        }
      }
      if (alarmEnabled) {
        printf("Received control word: ");
        alarmCount = 0;
        alarm(0);

        for (int i = 0; i < 5; i++) {
          printf("0x%02X ", bufR[i]);
        }
        break;
      }
    }

    return 0;
  } else {
    int nBytesBuf = 0;

    State state = Start;

    while (STOP == FALSE) {
      // Read one byte from serial port.
      // NOTE: You must check how many bytes were actually read by reading the
      // return value. In this example, we assume that the byte is always read,
      // which may not be true.
      unsigned char byte;
      int bytes = readByteSerialPort(&byte);
      nBytesBuf += bytes;

      switch (state) {
      case Start:
        printf("start\n");
        if (byte == FLAG)
          state = Flag_RCV;

        else
          state = Start;

        break;

      case Flag_RCV:
        printf("flag\n");
        if (byte == FLAG)
          state = Flag_RCV;

        else if (byte == add1)
          state = A_RCV;

        else
          state = Start;

        break;

      case A_RCV:
        printf("a\n");
        if (byte == FLAG)
          state = Flag_RCV;

        else if (byte == SET)
          state = C_RCV;

        else
          state = Start;

        break;

      case C_RCV:
        printf("c\n");
        if (byte == FLAG)
          state = Flag_RCV;

        else if (byte == 0)
          state = BCC_RCV;

        else
          state = Start;

        break;

      case BCC_RCV:

        printf("bcc\n");
        if (byte == FLAG) {
          state = Stop;
          STOP = TRUE;
        }

        else
          state = Start;
        break;

      case Stop:
        printf("stop\n");
        STOP = TRUE;
        break;
      default:
        break;
      }
    }

    printf("SENDING NOW");
    unsigned char buf[BUF_SIZE] = {0};

    buf[0] = FLAG;
    buf[1] = add1;
    buf[2] = UA;
    buf[3] = add1 ^ UA;
    buf[4] = FLAG;

    sleep(1);
    int bytes = writeBytesSerialPort(buf, 5);
    int i = 0;
    while (i < 5) {
      printf("var = 0x%02X\n", buf[i]);
      i++;
    }

    printf("Total bytes received: %d\n", nBytesBuf);

    return 0;
  }
}

void alarmHandler(int signal) {
  alarmEnabled = FALSE;
  alarmCount++;
  printf("Alarm #%d received\n", alarmCount);
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO: Implement this function

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    

    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    //
    // NOTE: See the i// code blockmplementation of the serial port library in "serial_port/".
    

    // Read from serial port until the 'z' char is received.

    // NOTE: This while() cycle is a simple example showing how to read from the serial port.
    // It must be changed in order to respect the specifications of the protocol indicated in the Lab guide.

    // TODO: Save the received bytes in a buffer array and print it at the end of the program.
    int nBytesBuf = 0;

    State state = Start;

    while (STOP == FALSE) {
      // Read one byte from serial port.
      // NOTE: You must check how many bytes were actually read by reading the
      // return value. In this example, we assume that the byte is always read,
      // which may not be true.
      unsigned char byte;
      int bytes = readByteSerialPort(&byte);
      nBytesBuf += bytes;

      switch (state) {
      case Start:
        printf("start\n");
        if (byte == FLAG)
          state = Flag_RCV;

        else
          state = Start;

        break;

      case Flag_RCV:
        printf("flag\n");
        if (byte == FLAG)
          state = Flag_RCV;

        else if (byte == add1)
          state = A_RCV;

        else
          state = Start;

        break;

      case A_RCV:
        printf("a\n");
        if (byte == FLAG)
          state = Flag_RCV;

        else if (byte == SET)
          state = C_RCV;

                else if(byte == 0xAA) state = C_INF_RCV;

        else
          state = Start;

        break;

      case C_RCV:
        printf("c\n");
        if (byte == FLAG)
          state = Flag_RCV;

        else if (byte == 0)
          state = BCC_RCV;

                else state = Start;
                
                break;

            case C_INF_RCV:
                printf("c_inf\n");
                if(byte == FLAG) state = Flag_RCV;
                
                else if (byte == (add1 ^ byte)) state = BCC_INF_RCV;
                
        else
          state = Start;

        break;

      case BCC_RCV:

        printf("bcc\n");
        if (byte == FLAG) {
          state = Stop;
          STOP = TRUE;
        }

        else
          state = Start;
        break;

            case BCC_INF_RCV:
                printf("bcc_inf\n");
                state = DATA_RCV;
                break;

            case DATA_RCV:
                printf("data\n");
                
                break;
            
          case Stop:
        printf("stop\n");
        STOP = TRUE;
        break;
      default:
        break;
      }
    }

    printf("SENDING NOW");
    unsigned char buf[BUF_SIZE] = {0};

    buf[0] = FLAG;
    buf[1] = add1;
    buf[2] = UA;
    buf[3] = add1 ^ UA;
    buf[4] = FLAG;

    sleep(1);
    int bytes = writeBytesSerialPort(buf, 5);
    int i = 0;
    while (i < 5) {
      printf("var = 0x%02X\n", buf[i]);
      i++;
    }

    printf("Total bytes received: %d\n", nBytesBuf);

    return 0;
  }



////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose()
{

    if (closeSerialPort() < 0)
    {
        perror("closeSerialPort");
    }
    return 0;
}
