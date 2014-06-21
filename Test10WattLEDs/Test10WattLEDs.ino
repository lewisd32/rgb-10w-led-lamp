/*
 * Demonstration code for the 6 10 Watt RGB LEDs that I currently have set up.
 *
 * The lights are set to 64 of 256 brightness, to avoid over heating.
 * A button allows making them brighter temporarily, for demonstrating how
 * bright they should eventually be.
 */

#include "Adafruit_TLC59711.h"
#include <SPI.h>
#include <hsl2rgb.h>
#include <PeripheralCommand.h>
#include <VLightOverride.h>
#include <brightness.h>
#include <VLight.h>
#include <Peripheral.h>
#include <Lamp.h>

const int MIN_BRIGHTNESS = 64;
const int FPS = 60;

#define NUM_TLC59711 2

#define BUTTON_PIN 2
#define data   11
#define clock  13

Adafruit_TLC59711 tlc = Adafruit_TLC59711(NUM_TLC59711, clock, data);

Lamp lamp(ledUpdateFunc);
PeripheralCommand cmd;

void setup() {
  Serial.begin(57600);
  Serial.println("Setting up");

  pinMode(BUTTON_PIN, INPUT);

  tlc.begin();
  
  lamp.init();
  tlc.write(); // Turn all the LEDs off initially.
  

  for (int i = 0; i < 12; ++i) {
    cmd.light = i+1;
    cmd.adjustments = PeripheralCommand::ADJUSTMENT_BRIGHTNESS |
      PeripheralCommand::ADJUSTMENT_HUE |
      PeripheralCommand::ADJUSTMENT_SATURATION |
      PeripheralCommand::ADJUSTMENT_ANGLE;
    cmd.isSetColors = true;
    cmd.isSetAngle = true;
    cmd.hsb[0] = 0;
    cmd.hsb[1] = 20;
    cmd.hsb[2] = MIN_BRIGHTNESS;
    cmd.angle = 15*i;
    lamp.peripheral[0].applyPeripheralCommand(cmd);
  }
  
  cmd.light = 0;
  cmd.adjustments = PeripheralCommand::ADJUSTMENT_BRIGHTNESS;
  cmd.isSetColors = true;
  cmd.hsb[0] = 0;
  cmd.hsb[1] = 0;
  cmd.hsb[2] = MIN_BRIGHTNESS;
  
  lamp.updateLeds();
  tlc.write();

  Serial.println("Setup complete");
}

uint32_t start = millis();

uint32_t count = 0;

void loop() {
  uint32_t end = millis() + 1000/FPS;
  
  if (digitalRead(BUTTON_PIN) == LOW) {
    // If the button was pressed, increase the brightness
    Serial.println("Pressed");
    if (cmd.hsb[2] < 64) {
      // Increase it quickly if it's quite dim
      cmd.hsb[2] += 10;
    } else if (cmd.hsb[2] < 250) {
      // Increase it more quickly while it's dim
      cmd.hsb[2] += 2;
    } else if (cmd.hsb[2] < 255) {
      // Increase it most slowly the last few, so it doesn't overflow.
      cmd.hsb[2] += 1;
    }
  } else {
    // If the button wasn't pressed, dim the light
    if (cmd.hsb[2] > MIN_BRIGHTNESS) {
      cmd.hsb[2] -= 7;
    }
    if (cmd.hsb[2] < MIN_BRIGHTNESS) {
      cmd.hsb[2] = MIN_BRIGHTNESS;
    }
  }
  
  // Execute the command we just set the brightness on
  lamp.peripheral[0].applyPeripheralCommand(cmd);
  
  lamp.updateLeds();
  tlc.write();

  while (millis() < end);
}

void ledUpdateFunc(uint8_t led, uint16_t red, uint16_t green, uint16_t blue) {
  tlc.setLED(led, 65535-(red*64), 65535-(green*64), 65535-(blue*64));
}

