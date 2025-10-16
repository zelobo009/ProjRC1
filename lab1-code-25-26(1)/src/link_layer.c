// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"
#include "../lab1/read.c"

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
    
    int nBytesBuf = 0;

    State state = Start;
    unsigned char ctrl = 0;
    unsigned char result[BUF_SIZE]; 
    int index = 0;
    STOP = FALSE;


    while (STOP == FALSE)
    {

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

                else {
                    ctrl = byte;
                    state = C_RCV;
                }
                break;

            case C_RCV:
                printf("c\n");
                if(byte == FLAG) state = Flag_RCV;

                else if (byte == (add1 ^ ctrl)) state = BCC_RCV;

                else state = Start;
                
                break;

            

            case BCC_RCV:

                printf("bcc\n");
                
                if(ctrl == SET){
                    
                    if(byte == FLAG){
                        
                        state = Stop;
                        STOP = TRUE;

                        unsigned char buf[5] = {0}; 

                        buf[0] = FLAG;
                        buf[1] = add1;
                        buf[2] = UA;
                        buf[3] = add1 ^ UA;
                        buf[4] = FLAG;

                        writeBytesSerialPort(buf, 5);
                    } 

                    else state = Start;
                }
                else if(ctrl == 0xAA){
                    if(byte == FLAG){
                        state = Stop;
                        STOP = TRUE;
                        
                    } 
                    else {
                        result[index++] = byte;
                        state = DATA_RCV;
                    }

                }


            case DATA_RCV:
                printf("data\n");
                
                if(byte == FLAG){
                    state = Stop;
                    STOP = TRUE;
                        
                } 
                else {
                    result[index++] = byte;
                }
                break;
            
            case Stop:
                printf("stop\n");
                STOP = TRUE;
                break;
            
            default:
                break;
        }

        
        }
        
        

    for (int i = 0; i < index; i++) {
        packet[i] = result[i];
    }

    

    printf("Received packet: ");
    for (int i = 0; i < index; i++) {
        printf("%02X ", packet[i]);
    }
    printf("\n");


    return index;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose()
{
    // TODO: Implement this function

    return 0;
}


