/*
 * This peripheral creates three lights, red, green, and blue, that
 * can be controlled with 3 knobs and a switch.  The switch toggles
 * between the knob directly controlling the angle of the light, or
 * controlling the speed at which the light rotates.
 */

#include <SPI.h>
#include <PeripheralCommand.h>

const int PIN_CS = 10;
const int PIN_MISO = 12;
const int PIN_MOSI = 11;
const int PIN_SCK = 13;
const int PIN_RTS = 9;


volatile byte cmdBuf[256];
volatile bool sent = true;
volatile byte len = 0;

void setup() {
  pinMode(PIN_CS, INPUT_PULLUP);
  pinMode(PIN_MISO, OUTPUT);
  pinMode(PIN_MOSI, INPUT);
  pinMode(PIN_SCK, INPUT);
  pinMode(PIN_RTS, OUTPUT);
  pinMode(2, INPUT_PULLUP);
  
  Serial.begin(115200);

  // turn on SPI in slave mode
  SPCR |= _BV(SPE);
  
  // now turn on interrupts
  SPI.attachInterrupt();
}

// The angles are stored as 10x the actual angle, to give one decimal
// of precision, to allow for 1/10th the movement speed.  Otherwise,
// 1 degree per frame (60 degrees per second) would be the minimum speed.
uint16_t rAngle = 0;
uint16_t bAngle = 0;
uint16_t gAngle = 0;

void loop (void) {
  Serial.println("Generating next command");
  PeripheralCommand* cmd = (PeripheralCommand*)(&cmdBuf);
  len = sizeof(PeripheralCommand) * 3;
  Serial.print("len = ");
  Serial.println(len);

  int16_t rKnob = ((int16_t)1023 - analogRead(0));
  int16_t bKnob = ((int16_t)1023 - analogRead(1));
  int16_t gKnob = ((int16_t)1023 - analogRead(2));
  
  if (digitalRead(2) == HIGH) {
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

  uint16_t checksum = len;
  for (int i = 0; i < len; ++i) {
    checksum += cmdBuf[i];
    Serial.print(cmdBuf[i]);
    Serial.print(' ');
  }
  
  cmdBuf[len] = ((checksum & 0xff00) >> 8);
  cmdBuf[len+1] = (checksum & 0xff);
  len += 2;

  Serial.println();
  Serial.print("Checksum: ");
  Serial.println(checksum);

  Serial.println("Ready");
  sent = false;
  while (!sent);
}

const byte START = 1;
const byte CHECK = 2;
const byte GET_LEN = 3;
const byte NEXT = 4;
const byte DONE = 5;

byte offset = 0;

// SPI interrupt routine
ISR (SPI_STC_vect)
{
//  Same as digitalWrite(PIN_RTS, HIGH) but faster
  PORTB |= 0x02; // set pin PB1, digital 9
  byte cmd = SPDR;
  switch(cmd) {
    case START:
      SPDR = sent?0:42;
      break; 
    case CHECK:
      SPDR = len;
      break;
    case GET_LEN:
      offset = 0;
      SPDR = cmdBuf[offset];
      break;
    case NEXT:
      ++offset;
      SPDR = cmdBuf[offset];
      break;
    case DONE:
      sent = true;
      break;
  }
  PORTB ^= 0x02; // clear pin PB1, digital 9
//  Same as digitalWrite(PIN_RTS, LOW) but faster
}

