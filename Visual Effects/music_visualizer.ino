#include <FastLED.h>
#include <arduinoFFT.h>

// --- Matrix Config ---
#define LED_PIN 6
#define ROWS 10
#define COLS 22
#define NUM_LEDS (ROWS * COLS)
#define BRIGHTNESS 128
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

// --- Audio & FFT Config ---
#define MIC_PIN A0
#define SAMPLES 64
#define SAMPLING_FREQUENCY 2000
unsigned int sampling_period_us;
float vReal[SAMPLES];
float vImag[SAMPLES];
ArduinoFFT<float> FFT = ArduinoFFT<float>(vReal, vImag, SAMPLES, SAMPLING_FREQUENCY);

// --- Bars & Peaks ---
#define NUM_BARS 7
uint8_t barHeights[NUM_BARS];
uint8_t oldBarHeights[NUM_BARS];
uint8_t peakDotY[NUM_BARS];
unsigned long peakDot_lastFall = 0;
const int PEAK_FALL_DELAY = 60;
float fftMax = 1;

// --- XY Mapping ---
uint16_t XY(uint8_t x, uint8_t y) {
  if (x >= COLS || y >= ROWS) return -1;
  return (y % 2 == 1) ? (y * COLS) + (COLS - 1 - x) : (y * COLS) + x;
}

void setup() {
  // Serial.begin(9600); // Commented out to save memory
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
  for (int i = 0; i < NUM_BARS; i++) {
    peakDotY[i] = 0;
    oldBarHeights[i] = 0;
  }
  fill_solid(leds, NUM_LEDS, CRGB::Black);
}

void fadeWithSparkle(uint8_t fadeBy = 50) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].fadeToBlackBy(fadeBy);
  }
}

void loop() {
  // --- 1. Sample Audio ---
  unsigned long microseconds;
  for (int i = 0; i < SAMPLES; i++) {
    microseconds = micros();
    vReal[i] = analogRead(MIC_PIN);
    vImag[i] = 0;
    while (micros() < (microseconds + sampling_period_us)) { /* wait */ }
  }

  // --- 2. FFT ---
  FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);
  FFT.compute(FFTDirection::Forward);
  FFT.complexToMagnitude();

  // --- 3. Bin Frequencies & Auto-Scale ---
  float currentMax = 0;
  for (int i = 0; i < NUM_BARS; i++) {
    float peak = 0;
    int start = pow(2, i) - 1;
    int end = pow(2, i + 1) - 1;
    if (end > SAMPLES / 2) end = SAMPLES / 2;
    for (int j = start; j < end; j++) if (vReal[j] > peak) peak = vReal[j];
    if (peak > currentMax) currentMax = peak;

    uint8_t targetHeight = constrain(map(peak, 0, fftMax, 0, ROWS), 0, ROWS);
    if (targetHeight > oldBarHeights[i])
      oldBarHeights[i] = targetHeight;
    else
      oldBarHeights[i] = max(0, oldBarHeights[i] - 1);
    barHeights[i] = oldBarHeights[i];
  }
  if (currentMax > fftMax) fftMax = currentMax;
  else fftMax *= 0.995;

  // --- 4. Draw Bars & Peak Dots ---
  fadeWithSparkle(50);

  for (int bar = 0; bar < NUM_BARS; bar++) {
    int startX = map(bar, 0, NUM_BARS, 0, COLS);
    int endX = map(bar + 1, 0, NUM_BARS, 0, COLS);

    for (int y = 0; y < barHeights[bar]; y++) {
      CHSV color = CHSV(map(bar, 0, NUM_BARS, 0, 192) + map(y, 0, ROWS, 0, 64) + millis() / 10, 255, 200 + random8(55));
      for (int x = startX; x < endX; x++) {
        leds[XY(x, y)] = CHSV(color.h + x * 3, color.s, color.v);
      }
    }

    if (barHeights[bar] > peakDotY[bar]) peakDotY[bar] = barHeights[bar];
    if (peakDotY[bar] > 0) {
      for (int x = startX; x < endX; x++) leds[XY(x, peakDotY[bar]-1)] = CRGB::White;
    }
  }

  // --- 4a. Bass Pulse Layer ---
  int bassLevel = max(barHeights[0], barHeights[1]);
  if (bassLevel > ROWS / 2) {
    for (int x = 0; x < COLS; x++) {
      leds[XY(x, 0)] += CHSV(160, 255, 255);
    }
  }

  // --- 5. Peak Dot Decay ---
  if (millis() - peakDot_lastFall > PEAK_FALL_DELAY) {
    peakDot_lastFall = millis();
    for (int i = 0; i < NUM_BARS; i++) if (peakDotY[i] > 0) peakDotY[i]--;
  }

  FastLED.show();
}