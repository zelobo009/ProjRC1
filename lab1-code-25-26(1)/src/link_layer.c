// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"
#include "lab1/read.c"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    // TODO: Implement this function

    return 0;
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
    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS0\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    //
    // NOTE: See the i// code blockmplementation of the serial port library in "serial_port/".
    const char *serialPort = argv[1];

    if (openSerialPort(serialPort, BAUDRATE) < 0)
    {
        perror("openSerialPort");
        exit(-1);
    }

    printf("Serial port %s opened\n", serialPort);

    // Read from serial port until the 'z' char is received.

    // NOTE: This while() cycle is a simple example showing how to read from the serial port.
    // It must be changed in order to respect the specifications of the protocol indicated in the Lab guide.

    // TODO: Save the received bytes in a buffer array and print it at the end of the program.
    int nBytesBuf = 0;

    State state = Start;



    while (STOP == FALSE)
    {
        // Read one byte from serial port.
        // NOTE: You must check how many bytes were actually read by reading the return value.
        // In this example, we assume that the byte is always read, which may not be true.
        unsigned char byte;
        int bytes = readByteSerialPort(&byte);
        nBytesBuf += bytes;




        
        switch (state) {
            case Start:
                printf("start\n");
                if(byte == FLAG) state = Flag_RCV;
                
                else state = Start;
                
                break;
            
            case Flag_RCV:
                printf("flag\n");
                if(byte == FLAG) state = Flag_RCV;

                else if(byte == add1) state = A_RCV;

                else state = Start;
                
                break;

            case A_RCV:
                printf("a\n");
                if(byte == FLAG) state = Flag_RCV;

                else if(byte == SET) state = C_RCV;

                else state = Start;
                
                break;

            case C_RCV:
                printf("c\n");
                if(byte == FLAG) state = Flag_RCV;

                else if(byte == 0) state = BCC_RCV;

                else state = Start;
                
                break;

            case BCC_RCV:

                printf("bcc\n");
                if(byte == FLAG){
                    state = Stop;
                    STOP = TRUE;
                } 

                else state = Start;
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
    while(i<5){
        printf("var = 0x%02X\n", buf[i]);
        i++;
    }


    printf("Total bytes received: %d\n", nBytesBuf);

    // Close serial port
    if (closeSerialPort() < 0)
    {
        perror("closeSerialPort");
        exit(-1);
    }

    printf("Serial port %s closed\n", serialPort);

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose()
{
    // TODO: Implement this function

    return 0;
}
