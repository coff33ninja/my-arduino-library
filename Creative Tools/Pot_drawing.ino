// Drawing Machine for 10x22 serpentine ARGB matrix
// Arduino Nano, 3 pots (X=A0, Y=A1, Color=A2), 1 button (D2)
// FastLED library

#include <FastLED.h>

#define WIDTH 22
#define HEIGHT 10
#define NUM_LEDS (WIDTH * HEIGHT)
#define DATA_PIN 6

#define POT_X_PIN A0
#define POT_Y_PIN A1
#define POT_COLOR_PIN A2
#define BUTTON_PIN 2

CRGB leds[NUM_LEDS];
uint8_t brightness = 64; // limit for power safety
bool serpentine = true;

bool firstFrame = true;

// pen state
bool penDown = false;
CHSV penColor = CHSV(0, 255, 255);

// cursor position
uint8_t curX = 0, curY = 0;

// track last cursor position
uint8_t lastX = 0, lastY = 0;
float smoothX = 0, smoothY = 0;
const float alpha = 0.15; // smoothing factor

// button debounce
int lastButtonState = HIGH;
int buttonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// long press detection for clear function
unsigned long buttonPressStartTime = 0;
bool buttonPressed = false;
const unsigned long longPressDuration = 2000; // 2 seconds for long press
bool longPressTriggered = false;

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(brightness);
  FastLED.clear(true);

  // start centered
  smoothX = (WIDTH - 1) / 2.0;
  smoothY = (HEIGHT - 1) / 2.0;
  curX = (uint8_t)round(smoothX);
  curY = (uint8_t)round(smoothY);
}

uint16_t XY_to_index(uint8_t x, uint8_t y) {
  if (!serpentine) return y * WIDTH + x;
  if (y % 2 == 0) {
    return y * WIDTH + x; // even row left→right
  } else {
    return y * WIDTH + (WIDTH - 1 - x); // odd row right→left
  }
}

void loop() {
  readPots();
  handleButton();
  render();
  FastLED.show();

  if (!firstFrame) {
    lastX = curX;
    lastY = curY;
  } else {
    lastX = curX;
    lastY = curY;
    firstFrame = false;
  }

  delay(12);
}

void readPots() {
  int rawX = analogRead(POT_X_PIN);
  int rawY = analogRead(POT_Y_PIN);
  int rawC = analogRead(POT_COLOR_PIN);

  // map pots
  float mappedX = (rawX / 1023.0) * (WIDTH - 1);
  float mappedY = (rawY / 1023.0) * (HEIGHT - 1);
  uint8_t hue = map(rawC, 0, 1023, 0, 255);

  // smoothing
  smoothX = (alpha * mappedX) + ((1 - alpha) * smoothX);
  smoothY = (alpha * mappedY) + ((1 - alpha) * smoothY);

  curX = constrain((uint8_t)round(smoothX), 0, WIDTH - 1);
  curY = constrain((uint8_t)round(smoothY), 0, HEIGHT - 1);
  penColor = CHSV(hue, 255, 255);
}

void drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, CRGB color) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy, e2;

  while (true) {
    // Boundary check before setting pixel
    if (x0 >= 0 && x0 < WIDTH && y0 >= 0 && y0 < HEIGHT) {
      leds[XY_to_index(x0, y0)] = color;
    }
    if (x0 == x1 && y0 == y1) break;
    e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

void render() {
  uint16_t idx = XY_to_index(curX, curY);
  CRGB cursorCol;
  hsv2rgb_rainbow(penColor, cursorCol);

  if (penDown) {
    // draw a line between last and current position
    drawLine(lastX, lastY, curX, curY, cursorCol);
  } else {
    // preview only, don’t overwrite
    CRGB old = leds[idx];
    if (old == CRGB::Black) {
      leds[idx] = cursorCol;
    } else {
      leds[idx] = old.nscale8(180) + cursorCol.nscale8(75);
    }
  }
}


void handleButton() {
  int reading = digitalRead(BUTTON_PIN);
  unsigned long now = millis();

  if (reading != lastButtonState) {
    lastDebounceTime = now;
  }

  if ((now - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      
      if (buttonState == LOW) {
        // Button just pressed
        buttonPressed = true;
        buttonPressStartTime = now;
        longPressTriggered = false;
      } else {
        // Button just released
        if (buttonPressed && !longPressTriggered) {
          // Short press - toggle pen state
          penDown = !penDown;
        }
        buttonPressed = false;
        longPressTriggered = false;
      }
    }
  }
  
  // Check for long press while button is held down
  if (buttonPressed && !longPressTriggered && 
      (now - buttonPressStartTime) >= longPressDuration) {
    // Long press detected - clear canvas
    clearCanvas();
    longPressTriggered = true;
  }

  lastButtonState = reading;
}

// Function to clear the entire canvas
void clearCanvas() {
  // Flash white briefly to indicate clear
  fill_solid(leds, NUM_LEDS, CRGB::White);
  FastLED.show();
  delay(100);
  
  // Clear to black
  FastLED.clear();
  FastLED.show();
  delay(100);
  
  // Flash white again
  fill_solid(leds, NUM_LEDS, CRGB::White);
  FastLED.show();
  delay(100);
  
  // Final clear
  FastLED.clear();
  FastLED.show();
}