#include <Arduino.h>
#include <FastLED.h>
#define DATA_PIN 3
#define NUM_LEDS 20
#define FAN1_LEDS 6
#define FAN2_LEDS 6
#define PUMP_LEDS 8

CRGB leds[NUM_LEDS];

// Temperature simulation (affects colors and speeds)
float temperature = 30.0; // Starting temp in °C
float targetTemp = 30.0;
bool overheating = false;

// Dynamic speeds based on temperature
float baseFan1Speed = 0.6;
float baseFan2Speed = -0.8; // opposite direction  
float basePumpSpeed = 0.4;

// Trail length (in LEDs)
int trailLength = 4;

// Dynamic colors based on temperature
CRGB getCoolantColor(float temp) {
  if (temp < 40) {
    // Cool: Deep blue to purple
    return blend(CRGB(0, 0, 255), CRGB(64, 0, 128), map(temp, 20, 40, 0, 255));
  } else if (temp < 60) {
    // Warm: Purple to orange
    return blend(CRGB(64, 0, 128), CRGB(255, 100, 0), map(temp, 40, 60, 0, 255));
  } else {
    // Hot: Orange to red
    return blend(CRGB(255, 100, 0), CRGB(255, 0, 0), map(temp, 60, 80, 0, 255));
  }
}

void setup() {
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(255);
  Serial.begin(9600);
  Serial.println("Liquid Cooler Simulation Started");
}

void loop() {
  static unsigned long lastMillis = 0;
  static unsigned long lastTempUpdate = 0;
  unsigned long currentMillis = millis();

  // Simulate temperature changes every 5 seconds
  if (currentMillis - lastTempUpdate >= 5000) {
    targetTemp = random(25, 75); // Random temp between 25-75°C
    lastTempUpdate = currentMillis;
    Serial.print("Target temperature: ");
    Serial.print(targetTemp);
    Serial.println("°C");
  }

  // Smooth temperature transition
  if (temperature < targetTemp) {
    temperature += 0.1;
  } else if (temperature > targetTemp) {
    temperature -= 0.1;
  }

  // Check overheating condition
  overheating = (temperature > 70);

  // Calculate dynamic speeds based on temperature
  float speedMultiplier = map(temperature, 20, 80, 1.0, 3.0);
  float fan1Speed = baseFan1Speed * speedMultiplier;
  float fan2Speed = baseFan2Speed * speedMultiplier;
  float pumpSpeed = basePumpSpeed * speedMultiplier;

  // Fade everything slightly each frame to create blur persistence
  uint8_t fadeAmount = overheating ? 20 : 40; // Less fade when overheating for intensity
  fadeToBlackBy(leds, NUM_LEDS, fadeAmount);

  // Draw spinning effects with temperature-based colors
  spinBlurChase(0, FAN1_LEDS, fan1Speed, temperature);
  spinBlurChase(FAN1_LEDS, FAN2_LEDS, fan2Speed, temperature);
  spinBlurChase(FAN1_LEDS + FAN2_LEDS, PUMP_LEDS, pumpSpeed, temperature);

  // Overheating warning flash
  if (overheating && (millis() / 200) % 2) {
    fill_solid(leds, NUM_LEDS, CRGB(255, 0, 0)); // Red flash warning
  }

  // Update LEDs ~60fps
  if (currentMillis - lastMillis >= 16) {
    FastLED.show();
    lastMillis = currentMillis;
  }
}

void spinBlurChase(int startIndex, int count, float speed, float temp) {
  float time = millis() / 1000.0;
  float pos = fmod(time * speed * count, count); // position in LEDs

  // Get temperature-based colors
  CRGB baseColor = getCoolantColor(temp);
  CRGB accentColor = getCoolantColor(temp + 10); // Slightly warmer accent

  // Draw main LED + trailing blur with temperature-based colors
  for (int t = 0; t < trailLength; t++) {
    float ledPos = pos - t;
    while (ledPos < 0) ledPos += count; // wrap backwards
    while (ledPos >= count) ledPos -= count; // wrap forwards

    // Blend between base and accent colors
    uint8_t blendAmount = (uint8_t)((ledPos / count) * 255);
    CRGB blendedColor = blend(baseColor, accentColor, blendAmount);

    // Fade brightness along the trail
    uint8_t brightness = 255 - (t * (255 / trailLength));
    
    // Extra intensity for overheating
    if (temp > 70) {
      brightness = min(255, brightness * 1.5);
    }
    
    leds[startIndex + (int)ledPos] += blendedColor.nscale8(brightness);
  }
}
