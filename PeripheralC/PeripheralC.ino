/*
 * This peripheral creates a single white light that rotates around the
 * ring at a fixed speed.
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

void loop (void) {
  Serial.println("Generating next command");
  uint8_t len = sizeof(PeripheralCommand) * 1;
  Serial.print("len = ");
  Serial.println(len);

  cmd[0].light = 1;
  cmd[0].rgb = false;
  cmd[0].isSetColors = true;
  cmd[0].adjustments = PeripheralCommand::ADJUSTMENT_HUE |
    PeripheralCommand::ADJUSTMENT_SATURATION |
    PeripheralCommand::ADJUSTMENT_BRIGHTNESS |
    PeripheralCommand::ADJUSTMENT_ANGLE;
  cmd[0].hue = 0;
  cmd[0].saturation = 0;
  cmd[0].brightness = 255;
  cmd[0].isSetAngle = false;
  cmd[0].angle = 1;

  Serial.println("Ready");
  sendCmd(cmdBuf, len);
}



