#ifndef lewisd_VLight
#define lewisd_VLight

#include "Arduino.h"

struct VLight {
  uint8_t id;
  uint8_t hsb[3];
  uint16_t angle : 9;
  VLightOverride override;

  VLight(uint16_t id = 0)
  : id(id), angle(0), override() {
    hsb[0] = 0;
    hsb[1] = 0;
    hsb[2] = 0;
  }

  void reset(uint8_t id) {
    this->id = id;
    hsb[0] = 0;
    hsb[1] = 0;
    hsb[2] = 0;
    angle = 0;
    override.reset();
  }
  
  void dump() {
    Serial.print("Light ID: ");
    Serial.println(id);
    Serial.print("HSB:      ");
    Serial.print(hsb[0]);
    Serial.print(", ");
    Serial.print(hsb[1]);
    Serial.print(", ");
    Serial.println(hsb[2]);
    Serial.print("angle:    ");
    Serial.println(angle);
  }
  
  void getRGB(uint16_t* rgb) {
    uint8_t rgb8[3];
    uint8_t luminance = brightnessToLuminance(hsb[2])/256;
    hsl2rgb( (uint16_t)hsb[0]*3,
      hsb[1],
      luminance,
//      hsb[2],
      rgb8);
    rgb[0] = rgb8[0]*4;
    rgb[1] = rgb8[1]*4;
    rgb[2] = rgb8[2]*4;

    /*
    Serial.print("HSB : ");
    Serial.print(hsb[0]);
    Serial.print(", ");
    Serial.print(hsb[1]);
    Serial.print(", ");
    Serial.print(hsb[2]);
    Serial.print(" (");
    Serial.print(luminance);
    Serial.print(")  RGB : ");
    

    Serial.print(rgb[0]);
    Serial.print(", ");
    Serial.print(rgb[1]);
    Serial.print(", ");
    Serial.print(rgb[2]);
    Serial.println();
    */
  }
  
  void applyPeripheralCommand(PeripheralCommand& command) {
//    Serial.print("Adjustments: ");
//    Serial.println(command.adjustments);
    if (command.rgb) {
      // not yet supported
    } else {
      // hsb command
      if (command.isSetColors) {
        for (int i = 0; i < 3; ++i) {
          if (command.adjustments & (1<<i)) {
            hsb[i] = command.hsb[i];
          }
        }
      } else if (command.isSubtractColors) {
        applySubtractCommand(command);
      } else {
        applyAddCommand(command);
      } 
    }
    if (command.adjustments & PeripheralCommand::ADJUSTMENT_ANGLE) {
      if (command.isSetAngle) {
        angle = command.angle;
        while (angle >= 360) {
          angle -= 360;
        }
      } else {
        if (command.angle > 0) {
          angle += command.angle;
          while (angle >= 360) {
            angle -= 360;
          }
        } else {
          int16_t angle = this->angle;
          angle += command.angle;
          while (angle < 0) {
            angle += 360;
          }
          this->angle = angle;
        }
      }
    }
  }
  
  void applySubtractCommand(PeripheralCommand& command) {
    if (command.wrapColors) {
      for (int i = 0; i < 3; ++i) {
        if (command.adjustments & (1<<i)) {
          hsb[i] -= command.hsb[i];
        }
      }
    } else {
      // don't wrap, limit.
      for (int i = 0; i < 3; ++i) {
        if (command.adjustments & (1<<i)) {
          if (command.hsb[i] > hsb[i]) {
            hsb[i] = 0;
          } else {
            hsb[i] -= command.hsb[i];
          }
        }
      }
    }
  }
 
  void applyAddCommand(PeripheralCommand& command) {
    if (command.wrapColors) {
      for (int i = 0; i < 3; ++i) {
        if (command.adjustments & (1<<i)) {
          hsb[i] += command.hsb[i];
        }
      }
    } else {
      // don't wrap, limit.
      for (int i = 0; i < 3; ++i) {
        if (command.adjustments & (1<<i)) {
          if (command.hsb[i] > 255-hsb[i]) {
            hsb[i] = 255;
          } else {
            hsb[i] += command.hsb[i];
          }
        }
      }
    }
  } 
  
};

#endif
// vim: set tabstop=2 shiftwidth=2 softtabstop=2 smarttab expandtab:

