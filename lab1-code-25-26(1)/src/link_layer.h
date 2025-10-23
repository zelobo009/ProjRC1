// Link layer header.
// DO NOT CHANGE THIS FILE

#ifndef _LINK_LAYER_H_
#define _LINK_LAYER_H_

typedef enum
{
    LlTx,
    LlRx,
} LinkLayerRole;

typedef struct
{
    char serialPort[50];
    LinkLayerRole role;
    int baudRate;
    int nRetransmissions;
    int timeout;
} LinkLayer;

typedef enum {Start, Flag_RCV, A_RCV, C_RCV,C_INF_RCV, BCC_RCV, BCC_DATA, DATA_RCV, Stop}State;

// Size of maximum acceptable payload.
// Maximum number of bytes that application layer should send to link layer.
#define MAX_PAYLOAD_SIZE 1000

// MISC
#define FALSE 0
#define TRUE 1
#define BUF_SIZE 256
#define FLAG 0x7E
#define add1 0x03
#define add2 0x01
#define SET 0x03
#define UA 0x07
#define DISC 0x0B

// Open a connection using the "port" parameters defined in struct linkLayer.
// Return 0 on success or -1 on error.
int llopen(LinkLayer connectionParameters);

// Send data in buf with size bufSize.
// Return number of chars written, or -1 on error.
int llwrite(const unsigned char *buf, int bufSize);

// Receive data in packet.
// Return number of chars read, or -1 on error.
int llread(unsigned char *packet);

// Close previously opened connection and print transmission statistics in the console.
// Return 0 on success or -1 on error.
int llclose();

// Creates a buffer with stuffing for flag and escape bytes
// Returns the number of bytes stuffed
int stuff_packet(unsigned char * stuffed_buf, const unsigned char * packet,int packetSize, unsigned char * bcc2);

// Destuffs the bytes of the result buffer and puts them in the packet buffer, and saves the given bcc2;
// Returns the size of the destuffed data packet
int byte_destuffing(unsigned char *packet, unsigned char *result, int packetBytes, unsigned char * bcc2, int dataSize);


// Checks if flag contained in buf is correct with the given paramaters
// Returns 1 if true or 0 if false
int check_Flag(unsigned char * buf, unsigned char add, unsigned char ctrl);

// State machine that tracks and reads a flag byte by byte into bufR
void receiveFlag(unsigned char * bufR, State * state, unsigned char byte);

// Changes the information packet number to the indicated by the ready receive sent by the Receiver
// Returns 1 if byte is valid or 0 if invalid
int changeCurrentPacket(unsigned char *CurrentPacket, unsigned char byte);



#endif // _LINK_LAYER_H_
