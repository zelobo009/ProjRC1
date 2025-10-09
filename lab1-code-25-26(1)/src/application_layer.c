// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"

#include <string.h>

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename) {
  LinkLayer connectionParameters;
  connectionParameters.baudRate = baudRate;
  strcpy(connectionParameters.serialPort, serialPort);
  if (strcmp("tx", role)) {
    connectionParameters.role = 0;
  } else {
    connectionParameters.role = 1;
  }
  connectionParameters.timeout = 3;
  connectionParameters.nRetransmissions = nTries;
  int r = llopen(connectionParameters);
  llclose();
  return;
}
