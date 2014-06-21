/*
 * This peripheral creates three lights, red, green, and blue, that
 * can be controlled with 3 knobs and a switch.  The switch toggles
 * between the knob directly controlling the angle of the light, or
 * controlling the speed at which the light rotates.
 */

#include <SPI.h>
#include <PeripheralCommand.h>
#include <PeripheralISR.h>

const int PIN_SWITCH = 2;

void setup() {
  Serial.begin(115200);

  pinMode(PIN_SWITCH, INPUT_PULLUP);
  setupPeripheral();
}

volatile byte cmdBuf[256];
PeripheralCommand* cmd = (PeripheralCommand*)(&cmdBuf);

// The angles are stored as 10x the actual angle, to give one decimal
// of precision, to allow for 1/10th the movement speed.  Otherwise,
// 1 degree per frame (60 degrees per second) would be the minimum speed.
uint16_t rAngle = 0;
uint16_t bAngle = 0;
uint16_t gAngle = 0;

void loop (void) {
  Serial.println("Generating next command");
  uint8_t len = sizeof(PeripheralCommand) * 3;
  Serial.print("len = ");
  Serial.println(len);

  int16_t rKnob = ((int16_t)1023 - analogRead(0));
  int16_t bKnob = ((int16_t)1023 - analogRead(1));
  int16_t gKnob = ((int16_t)1023 - analogRead(2));
  
  if (digitalRead(PIN_SWITCH) == HIGH) {
    // The knob controls the angle of the light directly
    rAngle = rKnob * 60 / 17;
    gAngle = gKnob * 60 / 17;
    bAngle = bKnob * 60 / 17;
  } else {
    // The knob controls the speed of the light
    rAngle = rAngle + rKnob/30;
    gAngle = gAngle + gKnob/30;
    bAngle = bAngle + bKnob/30;
    
    if (rAngle > 3600) {
      rAngle -= 3600;
    }
    if (gAngle > 3600) {
      gAngle -= 3600;
    }
    if (bAngle > 3600) {
      bAngle -= 3600;
    }
  }

  cmd[0].light = 1;
  cmd[0].rgb = false;
  cmd[0].isSetColors = true;
  cmd[0].adjustments = PeripheralCommand::ADJUSTMENT_HUE |
    PeripheralCommand::ADJUSTMENT_SATURATION |
    PeripheralCommand::ADJUSTMENT_BRIGHTNESS |
    PeripheralCommand::ADJUSTMENT_ANGLE;
  cmd[0].hue = 0;
  cmd[0].saturation = 255;
  cmd[0].brightness = 255;
  cmd[0].isSetAngle = true;
  cmd[0].angle = rAngle/10;
  
  cmd[1].light = 2;
  cmd[1].rgb = false;
  cmd[1].isSetColors = true;
  cmd[1].adjustments = PeripheralCommand::ADJUSTMENT_HUE |
    PeripheralCommand::ADJUSTMENT_SATURATION |
    PeripheralCommand::ADJUSTMENT_BRIGHTNESS |
    PeripheralCommand::ADJUSTMENT_ANGLE;
  cmd[1].hue = 256/3;
  cmd[1].saturation = 255;
  cmd[1].brightness = 255;
  cmd[1].isSetAngle = true;
  cmd[1].angle = bAngle/10;

  cmd[2].light = 3;
  cmd[2].rgb = false;
  cmd[2].isSetColors = true;
  cmd[2].adjustments = PeripheralCommand::ADJUSTMENT_HUE |
    PeripheralCommand::ADJUSTMENT_SATURATION |
    PeripheralCommand::ADJUSTMENT_BRIGHTNESS |
    PeripheralCommand::ADJUSTMENT_ANGLE;
  cmd[2].hue = 256/3*2;
  cmd[2].saturation = 255;
  cmd[2].brightness = 255;
  cmd[2].isSetAngle = true;
  cmd[2].angle = gAngle/10;

  Serial.println("Ready");
  sendCmd(cmdBuf, len);
}


