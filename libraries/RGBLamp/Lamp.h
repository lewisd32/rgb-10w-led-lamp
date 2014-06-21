#ifndef lewisd_Lamp
#define lewisd_Lamp

#include "Arduino.h"

#define debugPeripheralConnect false
#define debugCreateLight false
#define debugDeleteLight false


typedef void (*LedUpdateFunc)(uint8_t, uint16_t, uint16_t, uint16_t);

struct Lamp {
  public:
  static const uint8_t ledCount = 16;

  private:
  const static int maxPeripherals = 10;
  Peripheral peripheral[maxPeripherals];

  const static int maxVLights = 50;
  uint8_t vLightPort[maxVLights];
  VLight vLight[maxVLights];
  uint8_t lowestUnusedLight;

  static const uint16_t factor = 2;
  static const int16_t ledAngle = (int16_t)360*factor/ledCount;
  static const int16_t ledHalfAngle = ledAngle / 2;

  const LedUpdateFunc ledUpdateFunc;

  public:
  Lamp(LedUpdateFunc ledUpdateFunc) :
    lowestUnusedLight(0), ledUpdateFunc(ledUpdateFunc){
  }

  void init() {
    for (uint8_t led = 0; led < ledCount; ++led) {
      ledUpdateFunc(led, 0, 0, 0);
    }
  }

  void applyPeripheralCommand(int port, PeripheralCommand& command) {
    for (int i = 0; i < maxVLights; ++i) {
      VLight& light = vLight[i];
      if (light.id != 0) {
        if (vLightPort[i] == port) {
          if (command.light == light.id) {
            light.applyPeripheralCommand(command);
            return;
          }
          if (command.light == 0) {
            light.applyPeripheralCommand(command);
          }
        }
      }
    }
    if (command.light != 0) {
      // If we got this far, the light doesn't exist.
      int16_t i = createLight(port, command.light);
      if (i < 0) {
        Serial.print("Unable to create new light port:");
        Serial.print(port);
        Serial.print(" id:");
        Serial.println(command.light);
        return;
      }
      vLight[i].applyPeripheralCommand(command);
    }
  }

  void connect(uint8_t port) {
    if (!peripheral[port].connected) {
      if (debugPeripheralConnect) {
        Serial.print("Connected port ");
        Serial.println(port);
      }
      peripheral[port].connect();
    }
  }

  void disconnect(uint8_t port) {
    if (peripheral[port].connected) {
      if (debugPeripheralConnect) {
        Serial.print("Disconnected port ");
        Serial.println(port);
      }
      peripheral[port].disconnect();
      deleteLights(port, 0);
    }
  }

  uint8_t errors(uint8_t port, int8_t inc=0) {
    if (inc < 0) {
      if (peripheral[port].errors > -inc) {
        peripheral[port].errors += inc;
      } else {
        peripheral[port].errors = 0;
      }
    } else if (inc > 0) {
      if (peripheral[port].errors < 255-inc) {
        peripheral[port].errors += inc;
      } else {
        peripheral[port].errors = 255;
      }
    }
    return peripheral[port].errors;
  }

  void updateLeds() {
    uint16_t leds[ledCount][3];
    for (int led = 0; led < ledCount; ++led) {
        leds[led][0] = 0;
        leds[led][1] = 0;
        leds[led][2] = 0;
    }
    
    for (int i = 0; i < maxVLights; ++i) {
      //Serial.print("Iterating peripheral ");
      //Serial.println(i);
      VLight& light = vLight[i];
      if (light.id == 0) {
        continue;
      }
      //light.dump();
//      Serial.print("angle = ");
//      Serial.println(light.angle);
      uint8_t led = (uint16_t)light.angle*factor / ledAngle;
      //Serial.print("led = ");
      //Serial.println(led);
      int16_t offset = (int16_t)light.angle*factor - (int16_t)led*ledAngle - ledHalfAngle;
//      Serial.print("offset = ");
//      Serial.println(offset);
      uint16_t rgb[3];
      light.getRGB(rgb);
      
      applyVLight(rgb, leds[led], (uint16_t)(ledAngle-abs(offset))*1024/ledAngle);
      
      if (offset < 0) {
  uint8_t lowerLed = led==0?ledCount-1:led-1;
//        Serial.print("lowerLed = ");
//        Serial.println(lowerLed);
  applyVLight(rgb, leds[lowerLed], (uint16_t)abs(offset)*1024/ledAngle);
      } else {
  uint8_t upperLed = led==ledCount-1?0:led+1;
//        Serial.print("upperLed = ");
//        Serial.println(upperLed);
  applyVLight(rgb, leds[upperLed], (uint16_t)abs(offset)*1024/ledAngle);
      }

    }
    
    for (int led = 0; led < ledCount; ++led) {
      uint16_t red = leds[led][0];
      uint16_t green = leds[led][1];
      uint16_t blue = leds[led][2];
      ledUpdateFunc(led, red, green, blue);
    }
    /*
    for (int led = 0; led < ledCount; ++led) {
      Serial.print("LED ");
      if (led < 10) {
        Serial.print(' ');
      }
      Serial.print(led);
      Serial.print(" : ");
      Serial.print(leds[led][0]);
      Serial.print(", ");
      Serial.print(leds[led][1]);
      Serial.print(", ");
      Serial.println(leds[led][2]);
    }
    */
  }

  private:

  void applyVLight(uint16_t* rgb, uint16_t *led, uint32_t ratio) {
    ratio = min(ratio, 1023);
  //  Serial.print("ratio = ");
  //  Serial.println(ratio);
    uint16_t adjustedRatio = adjustRatio(ratio);
  //  Serial.print("adjustedRatio = ");
  //  Serial.println(adjustedRatio);
    for (uint8_t i = 0; i < 3; ++i) {
      led[i] = min(1023, (uint32_t)led[i] + (uint32_t)rgb[i]*adjustedRatio/1024);
    }
  }

  int16_t createLight(uint8_t port, uint8_t id) {
    if (debugCreateLight) {
      Serial.print("Creating light port:");
      Serial.print(port);
      Serial.print(" id:");
      Serial.println(id);
    }
    for (uint8_t i = lowestUnusedLight; i < maxVLights; ++i) {
      VLight& light = vLight[i];
      if (light.id == 0) {
        light.reset(id);
        vLightPort[i] = port;
        if (debugCreateLight) {
          Serial.print("index ");
          Serial.println(i);
        }
        return i;
      }
    }
    return -1;
  }

  void deleteLights(uint8_t port, uint8_t id) {
    for (uint8_t i = 0; i < maxVLights; ++i) {
      if (vLightPort[i] == port) {
        VLight& light = vLight[i];
        if (light.id == 0) {
          continue;
        }
        if (id == 0 || light.id == id) {
          if (debugDeleteLight) {
            Serial.print("Deleting light port:");
            Serial.print(port);
            Serial.print(" id:");
            Serial.print(light.id);
            Serial.print(" index:");
            Serial.println(i);
          }
          if (i < lowestUnusedLight) {
            lowestUnusedLight = i;
          }
          light.id = 0;
        }
      }
    }
  }

};

#endif
// vim: set tabstop=2 shiftwidth=2 softtabstop=2 smarttab expandtab:

