#ifndef lewisd_PeripheralISR
#define lewisd_PeripheralISR

#include "Arduino.h"

#define debugChecksum false

volatile byte* portCmdBuf;
volatile bool portCmdSent = true;
volatile byte portCmdLen = 0;

void setupPeripheral() {
  pinMode(10, INPUT_PULLUP);
  pinMode(12, OUTPUT);
  pinMode(11, INPUT);
  pinMode(13, INPUT);
  pinMode(9, OUTPUT);

  // turn on SPI in slave mode
  SPCR |= _BV(SPE);
  
  // now turn on interrupts
  SPI.attachInterrupt();
}

/*
 * cmdBuf must have room for an additional 2 bytes more than len,
 * for the checksum.
 */
void sendCmd(volatile byte* cmdBuf, uint8_t len) {
  uint16_t checksum = len;
  for (int i = 0; i < len; ++i) {
    checksum += cmdBuf[i];
    if (debugChecksum) {
      Serial.print(cmdBuf[i]);
      Serial.print(' ');
    }
  }
  
  cmdBuf[len] = ((checksum & 0xff00) >> 8);
  cmdBuf[len+1] = (checksum & 0xff);
  len += 2;

  if (debugChecksum) {
    Serial.println();
    Serial.print("Checksum: ");
    Serial.println(checksum);
  }

  portCmdBuf = cmdBuf;
  portCmdLen = len;
  portCmdSent = false;
  while (!portCmdSent);
}

// SPI interrupt routine
ISR (SPI_STC_vect)
{
  static byte offset = 0;
//  Same as digitalWrite(PIN_RTS, HIGH) but faster
  PORTB |= 0x02; // set pin PB1, digital 9
  byte cmd = SPDR;
  switch(cmd) {
    case START:
      SPDR = portCmdSent?0:42;
      break; 
    case CHECK:
      SPDR = portCmdLen;
      break;
    case GET_LEN:
      offset = 0;
      SPDR = portCmdBuf[offset];
      break;
    case NEXT:
      ++offset;
      SPDR = portCmdBuf[offset];
      break;
    case DONE:
      portCmdSent = true;
      break;
  }
  PORTB ^= 0x02; // clear pin PB1, digital 9
//  Same as digitalWrite(PIN_RTS, LOW) but faster
}

#endif
// vim: set tabstop=2 shiftwidth=2 softtabstop=2 smarttab expandtab:
