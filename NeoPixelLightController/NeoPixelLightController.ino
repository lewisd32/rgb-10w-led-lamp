/*
 * Main lamp control code.
 * Polls peripherals, reads commands from peripherals that are present,
 * and applies them to the lamp, updating the LEDs.
 * Uses an Adafruit 16 NeoPixel ring as a demo, rather than the 10 watt
 * RGB LEDs that will be in the lamp.
 */

#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <hsl2rgb.h>
#include <PeripheralCommand.h>
#include <VLightOverride.h>
#include <brightness.h>
#include <VLight.h>
#include <Peripheral.h>
#include <Lamp.h>

#define debugFrameTime false
#define debugPeripheralIOTime false
#define debugPeripheralReady false
#define debugPeripheralTimeout true

const int PIN_PS_A = 4; // Peripheral Select A
const int PIN_PS_B = 3; // Peripheral Select 2
const int PIN_PS_C = 2; // Peripheral Select 3
const int PIN_PE = 6; // Peripheral Enable

const int PIN_ERROR = 5;
const int PIN_NEO = 7;

const int PIN_CS = 10;
const int PIN_MISO = 12;
const int PIN_MOSI = 11;
const int PIN_SCK = 13;
const int PIN_RTS = 8;

const int PIN_PD = 9; // Peripheral Detect

const int FPS = 60;

const int CMD_BUF_LEN = 256;
byte cmdBuf[CMD_BUF_LEN];

const int MAX_ERRORS = 5;


Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, PIN_NEO, NEO_GRB + NEO_KHZ800);


void ledUpdateFunc(uint8_t led, uint16_t red, uint16_t green, uint16_t blue);

Lamp lamp(ledUpdateFunc);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting");
  Serial.print("Size of VLight: ");
  Serial.println(sizeof(VLight));
  
  pinMode(PIN_PS_A, OUTPUT);
  pinMode(PIN_PS_B, OUTPUT);
  pinMode(PIN_PS_C, OUTPUT);
  pinMode(PIN_PE, OUTPUT);
  pinMode(PIN_CS, OUTPUT);
  pinMode(PIN_MISO, INPUT);
  pinMode(PIN_MOSI, OUTPUT);
  pinMode(PIN_SCK, OUTPUT);
  pinMode(PIN_PD, INPUT);
  pinMode(PIN_RTS, INPUT);
  pinMode(PIN_ERROR, OUTPUT);

  SPI.setClockDivider(SPI_CLOCK_DIV8);
  SPI.begin();
  

  strip.begin();
  
  lamp.init(); // Initialize all pixels to 'off'
  strip.show();

  test();  
}

void test() {
  testRing(20, 1023, 0, 0);
  testRing(20, 0, 1023, 0);
  testRing(20, 0, 0, 1023);
  testRing(0, 0, 0, 0);
}

void testRing(uint16_t delayMs, uint16_t red, uint16_t green, uint16_t blue) {
  for (byte led = 0; led < lamp.ledCount; ++led) {
    ledUpdateFunc(led, red, green, blue);
    strip.show();
    delay(delayMs);
  }
}

void ledUpdateFunc(uint8_t led, uint16_t red, uint16_t green, uint16_t blue) {
  strip.setPixelColor(15-led, strip.Color(red/4, green/4, blue/4));
}

uint32_t errorStop = 0;

void loop() {
  uint32_t start = millis();
  uint32_t endFrame = start + (1000/FPS);
  if (millis() > errorStop) {
    digitalWrite(PIN_ERROR, LOW);
  }
//  Serial.println("Starting frame");
  for (byte port = 0; port < 3; ++port) {
    digitalWrite(PIN_PS_A, hasMaskToLevel(port, 0x01));
    digitalWrite(PIN_PS_B, hasMaskToLevel(port, 0x02));
    digitalWrite(PIN_PS_C, hasMaskToLevel(port, 0x04));
    digitalWrite(PIN_PE, LOW);
    
    int hasPeripheral = digitalRead(PIN_PD) == HIGH;
    if (hasPeripheral) {
//      Serial.print("Reading commands from peripheral ");
//      Serial.println(port);
      lamp.connect(port);
      digitalWrite(PIN_CS, LOW);
      int16_t len = readCmds();
      digitalWrite(PIN_CS, HIGH);
//      Serial.print("cmds len ");
//      Serial.println(len);
      if (len == -1) {
        // not ready
      } else if (len >= 0) {
        lamp.errors(-1);
        uint8_t offset = 0;
        while (offset + sizeof(PeripheralCommand) <= (uint8_t)len) {
          PeripheralCommand *pCmd = (PeripheralCommand*)(&cmdBuf[offset]);
          lamp.applyPeripheralCommand(port, *pCmd);
          offset += sizeof(PeripheralCommand);
        }
      } else {
        if (len == -100) {
          transferWait(5);
          SPI.transfer(DONE);
        }
        if (lamp.errors(port) > MAX_ERRORS) {
          Serial.println("fatal error");
          digitalWrite(PIN_ERROR, HIGH);
          errorStop = millis() + 500;
          lamp.disconnect(port);
        } else {
          lamp.errors(port, +1);
          digitalWrite(PIN_ERROR, HIGH);
          errorStop = millis() + 100;
          Serial.print("errors ");
          Serial.println(lamp.errors(port));
        }
      }
    } else {
      lamp.disconnect(port);
    }
    digitalWrite(PIN_PE, HIGH);
  }
  
  lamp.updateLeds();
  strip.show();
  
  uint32_t now = millis();
  if (now < endFrame) {
    if (debugFrameTime) {
      Serial.print("Frame time: ");
      Serial.println(now - start);
    }
    while (millis() < endFrame) {
      // wait for frame end
    }
  } else {
    // frame took too long.
    // TODO: Give some kind of indication, on a status LED?
    Serial.print("LONG FRAME: ");
    Serial.println(now - start);
  }
}

int hasMaskToLevel(byte input, byte mask) {
  if ((input & mask) > 0) {
    return HIGH;
  }
  return LOW;
}


int16_t readCmds() {
  uint32_t startMicros = micros();
  SPI.transfer(START);
  if (!transferWait(1)) {
    return -2;
  }
  byte ready = SPI.transfer(CHECK);
  if (ready != 42) {
    if (debugPeripheralReady) {
      Serial.print("Not ready ");
      Serial.println(ready);
    }
    return -1;
  }
  if (!transferWait(2)) {
    return -3;
  }
  byte len = SPI.transfer(GET_LEN);
//  Serial.print("len = ");
//  Serial.println(len);
  if (len > CMD_BUF_LEN) {
    return -4;
  }
  if (len < 2) {
    return -5;
  }
  uint16_t sum = len - 2;
  for (int i = 0; i < len; ++i) {
    if (!transferWait(3)) {
      return -6;
    }
    byte val = SPI.transfer(NEXT);
    cmdBuf[i] = val;
    sum += val;
//    Serial.print(val);
//    Serial.print(' ');
  }
//  Serial.println();
  byte checksumHigh = cmdBuf[len-2];
  byte checksumLow = cmdBuf[len-1];
  sum = sum - checksumHigh - checksumLow;
  uint16_t checksum = ((uint16_t)checksumHigh << 8) | checksumLow;
  uint32_t endMicros = micros();
  if (debugPeripheralIOTime) {
    Serial.print(endMicros - startMicros);
    Serial.println("us");
  }
  if (sum != checksum) {
    /*
    for (int i = 0; i < len; ++i) {
      Serial.print(cmdBuf[i]);
      Serial.print(' ');
    }
    Serial.println();
    Serial.print("Checksums: read=");
    Serial.print(checksum);
    Serial.print(" calc=");
    Serial.println(sum);
    Serial.println("************************************");
    Serial.println("**       Checksum Mismatch        **");
    Serial.println("************************************");
    */
    return -100;
  }
  if (!transferWait(4)) {
    return -7;
  }
  SPI.transfer(DONE);
  return len - 2;
}

bool transferWait(byte debugCode) {
  delayMicroseconds(5); // Give the slave a moment to get into the ISR (push) and raise RTS
  int32_t timeEnd = micros() + 1000;
// Same as while(digitalRead(PIN_RTS) == HIGH) but faster
  while((PORTB & 0x01) > 0) {
//    Serial.println(debugCode);
    if ((int32_t)micros() - timeEnd > 0) {
      if (debugPeripheralTimeout) {
        Serial.println("timeout");
      }
      return false;
    }
  }
  delayMicroseconds(9); // give the slave a moment to finish the ISR (pop)
  return true;
}
