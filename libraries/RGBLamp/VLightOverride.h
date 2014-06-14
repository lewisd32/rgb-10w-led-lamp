#ifndef lewisd_VLightOverride
#define lewisd_VLightOverride

#include "Arduino.h"

struct VLightOverride {
  int16_t angle : 9;
  bool active : 1;
  bool absoluteColor : 1;
  bool absoluteAngle : 1;
  int16_t hsb[3];
  
  VLightOverride()
  : angle(0), active(false), absoluteColor(false), absoluteAngle(false) {
    hsb[0] = 0;
    hsb[1] = 0;
    hsb[2] = 0;
  }
  
};

#endif

// vim: set tabstop=2 shiftwidth=2 softtabstop=2 smarttab expandtab:

