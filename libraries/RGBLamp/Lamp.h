#ifndef lewisd_Lamp
#define lewisd_Lamp

#include "Arduino.h"

typedef void (*LedUpdateFunc)(uint8_t, uint16_t, uint16_t, uint16_t);


struct Lamp {
  const static int maxPeripherals = 10;
  Peripheral peripheral[maxPeripherals];

  static const uint8_t ledCount = 16;
  static const uint16_t factor = 2;
  static const int16_t ledAngle = (int16_t)360*factor/ledCount;
  static const int16_t ledHalfAngle = ledAngle / 2;

  const LedUpdateFunc ledUpdateFunc;

  Lamp(LedUpdateFunc ledUpdateFunc) : ledUpdateFunc(ledUpdateFunc) {
    for (int i = 0; i < maxPeripherals; ++i) {
      peripheral[i].port = i;
    }
  }

  void init() {
    for (uint8_t led = 0; led < ledCount; ++led) {
      ledUpdateFunc(led, 0, 0, 0);
    }
  }


  void updateLeds() {
    uint16_t leds[ledCount][3];
    for (int led = 0; led < ledCount; ++led) {
        leds[led][0] = 0;
        leds[led][1] = 0;
        leds[led][2] = 0;
    }
    
    for (int i = 0; i < maxPeripherals; ++i) {
      //Serial.print("Iterating peripheral ");
      //Serial.println(i);
      VLight* light = peripheral[i].firstLight;
      while (light) {
        //light->dump();
  //      Serial.print("angle = ");
  //      Serial.println(light->angle);
        uint8_t led = (uint16_t)light->angle*factor / ledAngle;
        //Serial.print("led = ");
        //Serial.println(led);
        int16_t offset = (int16_t)light->angle*factor - (int16_t)led*ledAngle - ledHalfAngle;
  //      Serial.print("offset = ");
  //      Serial.println(offset);
        uint16_t rgb[3];
        light->getRGB(rgb);
        
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

        light = light->next;
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

};

#endif
// vim: set tabstop=2 shiftwidth=2 softtabstop=2 smarttab expandtab:

