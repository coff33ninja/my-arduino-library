/* --------------------------------------------------------------
   MatrixDriver – Arduino GPIO‑only driver (with extra commands)
   Enhanced version with additional features and error handling
   -------------------------------------------------------------- */

#include "Arduino.h"
#include <Arduino.h>
#include <FastLED.h>
#include <avr/pgmspace.h>

#define LED_PIN     6          // <‑‑ change here if you need another pin
#define ROWS        10
#define COLS        22
#define NUM_LEDS    (ROWS * COLS)
#define DEFAULT_BRIGHTNESS  32
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

// Protocol version for compatibility checking
#define PROTOCOL_VERSION 1

CRGB leds[NUM_LEDS];
bool matrixReady = false;
uint32_t frameCount = 0;
uint32_t bytesReceived = 0;

// ----------  Layout (serpentine – edit if you have a different wiring)
static uint16_t XY(uint8_t x, uint8_t y) {
  return (y * COLS) + ((y & 1) ? (COLS - 1 - x) : x);
}

// ----------  Serial protocol ----------
enum Cmd : uint8_t {
  CMD_FRAME     = 0xFF,   // full frame  (R,G,B …)
  CMD_PIXEL     = 0x01,   // single pixel (x,y,R,G,B)
  CMD_BRIGHT    = 0x02,   // set global brightness (0‑255)
  CMD_CLEAR     = 0x03,   // clear matrix to black
  CMD_FILL      = 0x04,   // fill matrix with color (R,G,B)
  CMD_STATUS    = 0x05,   // request status info
  CMD_TEST      = 0x06,   // run test pattern
  CMD_VERSION   = 0x07    // request version info
};

uint32_t lastRecv = 0;               // watchdog – clears after a while

// Status reporting
void sendStatus() {
  Serial.print("STATUS:");
  Serial.print(" LEDs="); Serial.print(NUM_LEDS);
  Serial.print(" Rows="); Serial.print(ROWS);
  Serial.print(" Cols="); Serial.print(COLS);
  Serial.print(" Brightness="); Serial.print(FastLED.getBrightness());
  Serial.print(" Frames="); Serial.print(frameCount);
  Serial.print(" Bytes="); Serial.print(bytesReceived);
  Serial.print(" Uptime="); Serial.print(millis());
  Serial.println();
}

// Test pattern
void runTestPattern() {
  // Rainbow test across all LEDs
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV((i * 256 / NUM_LEDS), 255, 255);
  }
  FastLED.show();
  Serial.println("TEST: Rainbow pattern displayed");
}

void receiveSerial() {
  static enum { WAIT_CMD, WAIT_FRAME, WAIT_PIXEL, WAIT_BRIGHT } state = WAIT_CMD;
  static uint16_t framePos = 0;      // bytes already stored for a frame

  while (Serial.available()) {
    uint8_t b = Serial.read();

    switch (state) {
      case WAIT_CMD:
        if (b == CMD_FRAME) { framePos = 0; state = WAIT_FRAME; }
        else if (b == CMD_PIXEL) { state = WAIT_PIXEL; }
        else if (b == CMD_BRIGHT) { state = WAIT_BRIGHT; }
        break;

      case WAIT_FRAME:          // fill the whole LED buffer
        {
          uint16_t idx = framePos / 3;
          uint8_t chan = framePos % 3;
          if (idx < NUM_LEDS) {
            if (chan == 0) leds[idx].r = b;
            else if (chan == 1) leds[idx].g = b;
            else leds[idx].b = b;
          }
          ++framePos;
          if (framePos >= NUM_LEDS * 3) {
            FastLED.show();
            state = WAIT_CMD;
          }
        }
        break;

      case WAIT_PIXEL:          // expect exactly 5 more bytes: x y r g b
        {
          static uint8_t pixBuf[5];
          static uint8_t pixIdx = 0;
          pixBuf[pixIdx++] = b;
          if (pixIdx == 5) {
            uint8_t x = pixBuf[0];
            uint8_t y = pixBuf[1];
            if (x < COLS && y < ROWS) {
              uint16_t i = XY(x, y);
              leds[i] = CRGB(pixBuf[2], pixBuf[3], pixBuf[4]);
              FastLED.show();               // immediate update of that pixel
            }
            pixIdx = 0;
            state = WAIT_CMD;
          }
        }
        break;

      case WAIT_BRIGHT:         // one byte = new brightness
        FastLED.setBrightness(b);
        FastLED.show();         // optional – forces an update
        state = WAIT_CMD;
        break;
    }
    lastRecv = millis();
  }

  // Optional watchdog: clear after 5 s of silence (helps after a disconnect)
  if (millis() - lastRecv > 5000) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    lastRecv = millis();   // only once
  }
}

// --------------------------------------------------------------
void setup() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(DEFAULT_BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  Serial.begin(115200);
  while (!Serial) ;          // wait for USB‑CDC enumeration
  
  // Startup sequence
  Serial.println("MatrixDriver v1.0 Ready");
  Serial.print("Matrix: "); Serial.print(COLS); Serial.print("x"); Serial.print(ROWS);
  Serial.print(" ("); Serial.print(NUM_LEDS); Serial.println(" LEDs)");
  Serial.println("Commands: FRAME(0xFF), PIXEL(0x01), BRIGHT(0x02), CLEAR(0x03), FILL(0x04), STATUS(0x05), TEST(0x06), VERSION(0x07)");
  
  // Brief startup indication
  fill_solid(leds, NUM_LEDS, CRGB(0, 32, 0)); // Dim green
  FastLED.show();
  delay(500);
  FastLED.clear();
  FastLED.show();
  
  matrixReady = true;
  lastRecv = millis();
}

void loop() {
  receiveSerial();           // non‑blocking – runs forever
}