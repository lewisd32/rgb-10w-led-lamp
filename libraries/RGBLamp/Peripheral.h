#ifndef lewisd_Peripheral
#define lewisd_Peripheral

#include "Arduino.h"

struct Peripheral {
  uint8_t port;
  uint8_t errors;
  
  VLight* firstLight;
  VLight* lastLight;
  
  Peripheral()
  : port(-1), errors(0), firstLight(NULL), lastLight(NULL) {
  }
  
  void applyPeripheralCommand(PeripheralCommand& command) {
    VLight* light = firstLight;
    bool applied = false;
    while (light) {
      if (command.light == 0 || command.light == light->id) {
        light->applyPeripheralCommand(command);
        applied = true;
      }
      light = light->next;
    }    
    if (!applied && command.light != 0) {
      Serial.print("Creating light ");
      Serial.println(command.light);
      light = createLight(command.light);
      light->applyPeripheralCommand(command);
    }
  }
  
  void dumpLights() {
    Serial.print("Dumping lights on peripheral port ");
    Serial.println(port);
    VLight* light = firstLight;
    while (light) {
      light->dump();
      light = light->next;
    }
    Serial.print("Dumping lights on peripheral port ");
    Serial.print(port);
    Serial.println(" in reverse");
    light = lastLight;
    while (light) {
      light->dump();
      light = light->prev;
    }
  }
  
  VLight* createLight(uint8_t id) {
    VLight* light = new VLight(id);
    if (lastLight) {
      lastLight->next = light;
      light->prev = lastLight;
    }
    lastLight = light;
    if (!firstLight) {
      firstLight = light;
    }
    return light;
  }
  
  VLight* deleteLight(VLight* light) {
    if (!light) {
      return NULL;
    }
    if (light->prev) {
      light->prev->next = light->next;
    } else {
      firstLight = light->next;
    }
    
    if (light->next) {
      light->next->prev = light->prev;
    } else {
      lastLight = light->prev;
    }
    
    delete light;
    return NULL;
  }
  
  void disconnect() {
    if (firstLight) {
      errors = 0;
      Serial.print("Disconnecting ");
      Serial.println(port);
      while (firstLight) {
        Serial.print("Deleting light ");
        Serial.println(firstLight->id);
        deleteLight(firstLight);
      }
    }
  }
  
};

#endif
// vim: set tabstop=2 shiftwidth=2 softtabstop=2 smarttab expandtab:

