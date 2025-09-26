// Ping Pong Game for 10x22 ARGB LED Matrix (Serpentine) with Angled Bounces
// Features: Dynamic ball speed, colored paddles, score-based brightness, optional LED scoreboard, angled ball bounces
// Hardware: Arduino Nano, WS2812B LEDs, 2 potentiometers (A0, A1), LED data on pin 6, reset button on pin 2

#include <FastLED.h>
#include <stdlib.h>  // For random() function

#define MATRIX_WIDTH 22
#define MATRIX_HEIGHT 10
#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT)
#define LED_PIN 6  // Data pin for LED strip
#define PADDLE_HEIGHT 3  // Height of each paddle in LEDs
#define BALL_SPEED_MIN 40  // Fastest ball speed (ms)
#define BALL_SPEED_START 100  // Starting ball speed (ms)
#define POT1_PIN A0  // Left player potentiometer
#define POT2_PIN A1  // Right player potentiometer
#define SCOREBOARD_ENABLED 1  // 1 for scores on top row, 0 for Serial
#define BOUNCE_ANGLES 1  // 1 for angled bounces, 0 for classic straight bounces
#define WINNING_SCORE 10 // Score to win the match
#define FADE_AMOUNT 40 // Amount to fade LEDs each frame for trails (0-255)
#define RESET_BUTTON_PIN 2 // Pin for reset button

CRGB leds[NUM_LEDS];  // LED array

// Game variables
int paddle1Y = 0;  // Top Y position of left paddle
int paddle2Y = 0;  // Top Y position of right paddle
float smoothedPot1 = 512; // Smoothed value, start in middle
float smoothedPot2 = 512; // Smoothed value, start in middle
const float alpha = 0.7;  // Smoothing factor (higher = more responsive, less smooth)
int ballX = MATRIX_WIDTH / 2;  // Ball X
int ballY = MATRIX_HEIGHT / 2;  // Ball Y
int ballDX = 1;  // Ball X direction (1 right, -1 left)
int ballDY = 1;  // Ball Y direction (-1 to 1, vertical speed)
int score1 = 0;  // Left player score
int score2 = 0;  // Right player score
int ballSpeed = BALL_SPEED_START;  // Dynamic ball speed
unsigned long lastUpdate = 0;  // For timing ball movement
int rallyCount = 0; // Number of consecutive paddle hits

// Reset button variables
bool lastResetButtonState = HIGH;
bool resetButtonState = HIGH;
unsigned long lastResetDebounceTime = 0;
const unsigned long resetDebounceDelay = 50;

// Forward declaration for resetBall
void resetBall(bool withAnimation);

// Function to get LED index from (x, y) in serpentine matrix
int getLEDIndex(int x, int y) {
  int index;
  if (y % 2 == 0) {
    index = y * MATRIX_WIDTH + x;
  } else {
    index = y * MATRIX_WIDTH + (MATRIX_WIDTH - 1 - x);
  }
  return index;
}

// Function to set a pixel color
void setPixel(int x, int y, CRGB color) {
  if (x >= 0 && x < MATRIX_WIDTH && y >= 0 && y < MATRIX_HEIGHT) {
    leds[getLEDIndex(x, y)] = color;
  }
}

// Function to clear the matrix
void clearMatrix() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
}

// Flash the screen white to celebrate a score
void flashScreen() {
  fill_solid(leds, NUM_LEDS, CRGB::White);
  FastLED.show();
  delay(50);
}

// A rainbow wave animation for when the ball resets
void rainbowWave() {
  uint8_t initialHue = beat8(10);
  for (int i = 0; i < MATRIX_WIDTH; i++) {
    fill_rainbow(leds, NUM_LEDS, initialHue + i * 4, 10);
    FastLED.show();
    delay(15);
  }
}

// Function to draw paddles
void drawPaddles() {
  // Left paddle (blue, column 0)
  for (int i = 0; i < PADDLE_HEIGHT; i++) {
    setPixel(0, paddle1Y + i, CRGB::Blue);
  }
  // Right paddle (green, column MATRIX_WIDTH - 1)
  for (int i = 0; i < PADDLE_HEIGHT; i++) {
    setPixel(MATRIX_WIDTH - 1, paddle2Y + i, CRGB::Green);
  }
}

// Function to draw ball
void drawBall() {
  setPixel(ballX, ballY, CRGB::Red);
}

// Function to handle reset button with debouncing
bool checkResetButton() {
  int reading = digitalRead(RESET_BUTTON_PIN);
  unsigned long currentTime = millis();
  
  if (reading != lastResetButtonState) {
    lastResetDebounceTime = currentTime;
  }
  
  if ((currentTime - lastResetDebounceTime) > resetDebounceDelay) {
    if (reading != resetButtonState) {
      resetButtonState = reading;
      if (resetButtonState == LOW) {  // Button pressed (active low)
        lastResetButtonState = reading;
        return true;
      }
    }
  }
  
  lastResetButtonState = reading;
  return false;
}

// Function to draw scoreboard on top row (if enabled)
void drawScoreboard() {
  #if SCOREBOARD_ENABLED
  // Player 1 score: columns 0-9 (Blue for units, Red for tens)
  int p1_tens = score1 / 10;
  int p1_units = score1 % 10;
  for (int i = 0; i < p1_tens; i++) {
    if (i < 10) setPixel(i, 0, CRGB::Red);
  }
  for (int i = 0; i < p1_units; i++) {
    if ((p1_tens + i) < 10) setPixel(p1_tens + i, 0, CRGB::Blue);
  }

  // Player 2 score: columns 12-21 (Green for units, Yellow for tens)
  int p2_tens = score2 / 10;
  int p2_units = score2 % 10;
  for (int i = 0; i < p2_tens; i++) {
    if (i < 10) setPixel(12 + i, 0, CRGB::Yellow);
  }
  for (int i = 0; i < p2_units; i++) {
    if ((p2_tens + i) < 10) setPixel(12 + p2_tens + i, 0, CRGB::Green);
  }
  #endif
}

// Function to update ball position and check collisions
void updateBall() {
  // Move ball
  ballX += ballDX;
  ballY += ballDY;

  // Debugging output for ball position and speed
  #if !SCOREBOARD_ENABLED
  Serial.print("Ball: ("); Serial.print(ballX); Serial.print(", "); Serial.print(ballY);
  Serial.print(") Speed: "); Serial.println(ballSpeed);
  #endif

  // Wall collisions (top/bottom) with clamping
  int minY = SCOREBOARD_ENABLED ? 1 : 0;
  if (ballY < minY) {
    ballY = minY;
    ballDY = -ballDY;
  } else if (ballY >= MATRIX_HEIGHT) {
    ballY = MATRIX_HEIGHT - 1;
    ballDY = -ballDY;
  }

  // Paddle collisions with angled bounces
  #if BOUNCE_ANGLES
  if (ballX == 0 && ballY >= paddle1Y && ballY < paddle1Y + PADDLE_HEIGHT) {
    // Hit left paddle: flash paddle white
    for (int i = 0; i < PADDLE_HEIGHT; i++) {
      setPixel(0, paddle1Y + i, CRGB::White);
    }
    FastLED.show();
    delay(10);

    ballDX = 1; // Go right
    ballX += ballDX;
    int hitPos = ballY - paddle1Y;  // 0 (top), 1 (middle), 2 (bottom)
    if (hitPos == 0) ballDY = -1;
    else if (hitPos == 1) ballDY = (random(0, 10) < 2) ? 0 : (random(0, 2) ? 1 : -1); // 20% chance of neutral
    else ballDY = 1;
    
    rallyCount++;
    if (rallyCount >= 4 && ballSpeed > BALL_SPEED_MIN) {
      ballSpeed -= 5;
      rallyCount = 0;
    }
  } else if (ballX == MATRIX_WIDTH - 1 && ballY >= paddle2Y && ballY < paddle2Y + PADDLE_HEIGHT) {
    // Hit right paddle: flash paddle white
    for (int i = 0; i < PADDLE_HEIGHT; i++) {
      setPixel(MATRIX_WIDTH - 1, paddle2Y + i, CRGB::White);
    }
    FastLED.show();
    delay(10);

    ballDX = -1; // Go left
    ballX += ballDX;
    int hitPos = ballY - paddle2Y;
    if (hitPos == 0) ballDY = -1;
    else if (hitPos == 1) ballDY = (random(0, 10) < 2) ? 0 : (random(0, 2) ? 1 : -1); // 20% chance of neutral
    else ballDY = 1;

    rallyCount++;
    if (rallyCount >= 4 && ballSpeed > BALL_SPEED_MIN) {
      ballSpeed -= 5;
      rallyCount = 0;
    }
  }
  #else
  // Classic straight bounce
  if (ballX == 0 && ballY >= paddle1Y && ballY < paddle1Y + PADDLE_HEIGHT) {
    // Hit left paddle: flash paddle white
    for (int i = 0; i < PADDLE_HEIGHT; i++) {
      setPixel(0, paddle1Y + i, CRGB::White);
    }
    FastLED.show();
    delay(10);
    
    ballDX = -ballDX;
    ballX += ballDX;
    
    rallyCount++;
    if (rallyCount >= 4 && ballSpeed > BALL_SPEED_MIN) {
      ballSpeed -= 5;
      rallyCount = 0;
    }
  } else if (ballX == MATRIX_WIDTH - 1 && ballY >= paddle2Y && ballY < paddle2Y + PADDLE_HEIGHT) {
    // Hit right paddle: flash paddle white
    for (int i = 0; i < PADDLE_HEIGHT; i++) {
      setPixel(MATRIX_WIDTH - 1, paddle2Y + i, CRGB::White);
    }
    FastLED.show();
    delay(10);

    ballDX = -ballDX;
    ballX += ballDX;

    rallyCount++;
    if (rallyCount >= 4 && ballSpeed > BALL_SPEED_MIN) {
      ballSpeed -= 5;
      rallyCount = 0;
    }
  }
  #endif

  // Score points
  if (ballX < 0) {
    score2++;
    rallyCount = 0;
    #if !SCOREBOARD_ENABLED
    Serial.print("Score: P1 "); Serial.print(score1); Serial.print(" - P2 "); Serial.println(score2);
    #endif
    flashScreen();
    resetBall(true);
  } else if (ballX >= MATRIX_WIDTH) {
    score1++;
    rallyCount = 0;
    #if !SCOREBOARD_ENABLED
    Serial.print("Score: P1 "); Serial.print(score1); Serial.print(" - P2 "); Serial.println(score2);
    #endif
    flashScreen();
    resetBall(true);
  }
}

// Reset ball to center with random direction, preventing neutral start
void resetBall(bool withAnimation) {
  if (withAnimation) {
    rainbowWave();
  }
  ballX = MATRIX_WIDTH / 2;
  ballY = random(SCOREBOARD_ENABLED ? 1 : 0, MATRIX_HEIGHT);
  ballDX = random(0, 2) == 0 ? -1 : 1;
  do {
    ballDY = random(-1, 2); // Generates -1, 0, or 1
  } while (ballDY == 0); // Ensures ball is not stuck in neutral
  ballSpeed = BALL_SPEED_START;
  rallyCount = 0;
}

void setup() {
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(30);
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  Serial.begin(115200);
  randomSeed(analogRead(A3));  // A3 floating
  resetBall(false);
}

void loop() {
  // Check for reset button
  if (checkResetButton()) {
    // Reset game immediately
    score1 = 0;
    score2 = 0;
    ballSpeed = BALL_SPEED_START;
    FastLED.setBrightness(30);
    
    // Quick flash to indicate reset
    fill_solid(leds, NUM_LEDS, CRGB::Purple);
    FastLED.show();
    delay(100);
    FastLED.clear();
    FastLED.show();
    delay(100);
    
    resetBall(false);
    
    #if !SCOREBOARD_ENABLED
    Serial.println("Game Reset! Score: P1 0 - P2 0");
    #endif
  }
  
  // Check for win condition
  if (score1 >= WINNING_SCORE || score2 >= WINNING_SCORE) {
    FastLED.clear();
    // Simple win animation: flash winning color
    CRGB winColor = (score1 >= WINNING_SCORE) ? CRGB::Blue : CRGB::Green;
    for(int i=0; i<5; i++) {
      fill_solid(leds, NUM_LEDS, winColor);
      FastLED.show();
      delay(200);
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      FastLED.show();
      delay(200);
    }
    score1 = 0;
    score2 = 0;
    resetBall(false);
  }

  // Read potentiometers with dummy reads to allow ADC to settle
  analogRead(POT1_PIN); // Dummy read
  int pot1 = analogRead(POT1_PIN);
  analogRead(POT2_PIN); // Dummy read
  int pot2 = analogRead(POT2_PIN);
  paddle1Y = map(pot1, 0, 1023, SCOREBOARD_ENABLED ? 1 : 0, MATRIX_HEIGHT - PADDLE_HEIGHT);
  paddle2Y = map(pot2, 0, 1023, SCOREBOARD_ENABLED ? 1 : 0, MATRIX_HEIGHT - PADDLE_HEIGHT);

  // Update ball
  if (millis() - lastUpdate > ballSpeed) {
    updateBall();
    lastUpdate = millis();
  }

  // Update brightness based on score (capped for power safety)
  FastLED.setBrightness(min(30 + (score1 + score2) * 5, 100));

  // Draw everything
  fadeToBlackBy(leds, NUM_LEDS, FADE_AMOUNT);
  drawPaddles();
  drawBall();
  drawScoreboard();
  FastLED.show();
}