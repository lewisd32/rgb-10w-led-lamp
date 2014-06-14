#ifndef lewisd_PeripheralCommand
#define lewisd_PeripheralCommand

#include "Arduino.h"

struct  __attribute__((__packed__)) PeripheralCommand {
  static const uint8_t ADJUSTMENT_RED = 1;
  static const uint8_t ADJUSTMENT_HUE = 1;
  static const uint8_t ADJUSTMENT_GREEN = 2;
  static const uint8_t ADJUSTMENT_SATURATION = 2;
  static const uint8_t ADJUSTMENT_BLUE = 4;
  static const uint8_t ADJUSTMENT_BRIGHTNESS = 4;
  static const uint8_t ADJUSTMENT_ANGLE = 8;
  
  uint8_t light;
  bool rgb : 1;
  bool isSetColors : 1;
  bool isSubtractColors : 1;
  bool wrapColors : 1;
  uint8_t adjustments : 4;

  bool isSetAngle : 1;
  int16_t angle : 10;
  
  union {
    uint8_t hsb[3];
    struct {
      uint8_t red;
      uint8_t green;
      uint8_t blue;
    };
    struct {
      uint8_t hue;
      uint8_t saturation;
      uint8_t brightness;
    };
  };
  
  PeripheralCommand() 
  : light(0), rgb(0), isSetColors(0), isSubtractColors(1), wrapColors(0), adjustments(0), isSetAngle(0), angle(0)
  {
    hsb[0] = 0;
    hsb[1] = 0;
    hsb[2] = 0;
  }
};

#endif
