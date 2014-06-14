/*
 * This peripheral creates a single white light that rotates around the
 * ring at a fixed speed.
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
  delay(1000);

  // turn on SPI in slave mode
  SPCR |= _BV(SPE);
  
  // now turn on interrupts
  SPI.attachInterrupt();

}

void loop (void) {
  Serial.println("Generating next command");
  PeripheralCommand* cmd = (PeripheralCommand*)(&cmdBuf);
  len = sizeof(PeripheralCommand) * 1;
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

