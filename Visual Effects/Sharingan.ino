#include <FastLED.h>
#include <avr/pgmspace.h>

#define LED_PIN     6
#define NUM_LEDS    220
#define BRIGHTNESS  32
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

#define ROWS 10
#define COLS 22

enum Mode {
  SHOW_SHARINGAN
};

Mode currentMode = SHOW_SHARINGAN;

// XY mapping function (adjust if your matrix is zigzag)
uint16_t XY(uint8_t x, uint8_t y) {
  if (y % 2 == 0) {
    return (y * COLS) + x;
  } else {
    return (y * COLS) + (COLS - 1 - x);
  }
}

// === Tomoe Drawing ===
void drawTomoe(int cx, int cy, float angle, CRGB color) {
  float rad = angle * (PI / 180.0);
  int x = cx + round(5 * cos(rad));
  int y = cy + round(3 * sin(rad));
  if (x >= 0 && x < COLS && y >= 0 && y < ROWS) {
    leds[XY(x,y)] = color;
  }
  if (x+1 < COLS) leds[XY(x+1, y)] = color;
}

// === Sharingan Mode ===
void showSharingan() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  int cx = COLS / 2;  // center x
  int cy = ROWS / 2;  // center y

  // Pulsing red iris
  uint8_t redLevel = beatsin8(6, 100, 255); 
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      int dx = x - cx;
      int dy = y - cy;
      int dist2 = dx*dx + dy*dy;

      if (dist2 < 20) { 
        leds[XY(x,y)] = CRGB(redLevel, 0, 0); // iris
      }
    }
  }

  // Black pupil (center)
  for (int y = cy-1; y <= cy+1; y++) {
    for (int x = cx-1; x <= cx+1; x++) {
      leds[XY(x,y)] = CRGB::Black;
    }
  }

  // Spinning tomoe
  static uint8_t rotation = 0;
  rotation += 3; // speed
  drawTomoe(cx, cy, rotation, CRGB::Black);
  drawTomoe(cx, cy, rotation + 120, CRGB::Black);
  drawTomoe(cx, cy, rotation + 240, CRGB::Black);

  // Flickering white glint
  if (random8() < 10) {
    int gx = cx + random8(-2, 3);
    int gy = cy - 2;
    if (gx >= 0 && gx < COLS && gy >= 0 && gy < ROWS) {
      leds[XY(gx,gy)] = CRGB::White;
    }
  }

  FastLED.show();
}
void setup() {
  delay(3000); // power-up safety delay
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
}
void loop() {
  switch (currentMode) {
    case SHOW_SHARINGAN:
      showSharingan();
      break;
  }
}
