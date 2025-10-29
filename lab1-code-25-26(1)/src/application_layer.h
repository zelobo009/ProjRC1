// Application layer protocol header.
// DO NOT CHANGE THIS FILE

#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

#include <stdio.h>


// Application layer main function.
// Arguments:
//   serialPort: Serial port name (e.g., /dev/ttyS0).
//   role: Application role {"tx", "rx"}.
//   baudrate: Baudrate of the serial port.
//   nTries: Maximum number of frame retries.
//   timeout: Frame timeout.
//   filename: Name of the file to send / receive.
void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename);

// Build control packet.
// Arguments:
//   cbuf: Buffer to store the control packet.
//   filename: Name of the file to send.
//   filesize: Size of the file to send.
// Returns: Size of the control packet.
int buildCtrlPacket(unsigned char *cbuf, const char *filename, size_t filesize);

// Read control packet.
// Arguments:
//   cbuf: Buffer containing the control packet.
//   rfilename: Buffer to store the received filename.
//   rfilesize: Pointer to store the received filesize.
// Returns: Control field of the packet.
int readCtrlPacket(unsigned char *cbuf, char *rfilename, size_t *rfilesize);

#endif // _APPLICATION_LAYER_H_
