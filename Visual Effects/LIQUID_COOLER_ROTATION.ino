#include <FastLED.h>
#define DATA_PIN 3
#define NUM_LEDS 20
#define FAN1_LEDS 6
#define FAN2_LEDS 6
#define PUMP_LEDS 8

CRGB leds[NUM_LEDS];

// Spin speeds (rotations per second)
float fan1Speed = 0.6;
float fan2Speed = -0.8; // opposite direction
float pumpSpeed = 0.4;

// Trail length (in LEDs)
int trailLength = 4;

// Colors for chase (deep purple + rich blue)
CRGB chaseColor1 = CRGB(64, 0, 128); // Deep royal purple
CRGB chaseColor2 = CRGB(0, 0, 255);  // Strong blue

void setup() {
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(255);
}

void loop() {
  static unsigned long lastMillis = 0;
  unsigned long currentMillis = millis();

  // Fade everything slightly each frame to create blur persistence
  fadeToBlackBy(leds, NUM_LEDS, 40);

  // Draw spinning effects
  spinBlurChase(0, FAN1_LEDS, fan1Speed);
  spinBlurChase(FAN1_LEDS, FAN2_LEDS, fan2Speed);
  spinBlurChase(FAN1_LEDS + FAN2_LEDS, PUMP_LEDS, pumpSpeed);

  // Update LEDs ~60fps
  if (currentMillis - lastMillis >= 16) {
    FastLED.show();
    lastMillis = currentMillis;
  }
}

void spinBlurChase(int startIndex, int count, float speed) {
  float time = millis() / 1000.0;
  float pos = fmod(time * speed * count, count); // position in LEDs

  // Draw main LED + trailing blur with color blend
  for (int t = 0; t < trailLength; t++) {
    float ledPos = pos - t;
    while (ledPos < 0) ledPos += count; // wrap backwards
    while (ledPos >= count) ledPos -= count; // wrap forwards

    // Blend between deep purple and blue
    uint8_t blendAmount = (uint8_t)((ledPos / count) * 255);
    CRGB blendedColor = blend(chaseColor1, chaseColor2, blendAmount);

    // Fade brightness along the trail
    uint8_t brightness = 255 - (t * (255 / trailLength));
    leds[startIndex + (int)ledPos] += blendedColor.nscale8(brightness);
  }
}
