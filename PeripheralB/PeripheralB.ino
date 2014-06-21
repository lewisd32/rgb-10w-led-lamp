/*
 * This peripheral creates a light (well, 16 seperate lights) spread
 * evenly around the ring, that pulses an adjustable colour, at an
 * adjustable speed.
 */
#include <SPI.h>
#include <PeripheralCommand.h>
#include <PeripheralISR.h>

void setup() {
  Serial.begin(115200);

  setupPeripheral();
}

volatile byte cmdBuf[256];
PeripheralCommand* cmd = (PeripheralCommand*)(&cmdBuf);

bool goingUp = true;
int16_t brightness = 200*10;

void loop (void) {
  Serial.println("Generating next command");
  uint8_t len = sizeof(PeripheralCommand) * 16;
  Serial.print("len = ");
  Serial.println(len);
  
  uint16_t hue = (1023 - analogRead(2))/4;
  uint16_t saturation = (1023 - analogRead(1))/4;
  uint16_t speed = (1023 - analogRead(0))/4;

  for (byte i = 0; i < 16; ++i) {
    cmd[i].light = i+1;
    cmd[i].rgb = false;
    cmd[i].isSetColors = true;
    cmd[i].adjustments = PeripheralCommand::ADJUSTMENT_HUE |
      PeripheralCommand::ADJUSTMENT_SATURATION |
      PeripheralCommand::ADJUSTMENT_BRIGHTNESS |
      PeripheralCommand::ADJUSTMENT_ANGLE;
    cmd[i].hue = hue;
    cmd[i].saturation = saturation;
    cmd[i].brightness = max(96, min(200, brightness/10));
    cmd[i].isSetAngle = true;
    cmd[i].angle = (uint16_t)i * 45 / 2;
  }

  if (goingUp) {
    brightness += speed;
    if (brightness/10 >= 255) {
      brightness = 255*10;
      goingUp = false;
    }
  } else {
    brightness -= speed;
    if (brightness/10 <= 0) {
      brightness = 0;
      goingUp = true;
    }
  }

  Serial.println("Ready");
  sendCmd(cmdBuf, len);
}


