#ifndef lewisd_Peripheral
#define lewisd_Peripheral

#include "Arduino.h"

struct Peripheral {
  uint8_t errors;
  bool connected;
  
  Peripheral()
  : errors(0), connected(false) {
  }

  void connect() {
    connected = true;
    errors = 0;
  }
  
  void disconnect() {
    connected = false;
    errors = 0;
  }
  
};

#endif
// vim: set tabstop=2 shiftwidth=2 softtabstop=2 smarttab expandtab:

