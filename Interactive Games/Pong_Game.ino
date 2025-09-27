/****************************************************************************************
 * Ping‑Pong Game for 10×22 ARGB LED Matrix (Serpentine) with Angled Bounces
 * –  Paddle flash replaces full‑screen flash on scoring
 * –  Rainbow animation is now limited to the paddles (called on a reset)
 *
 * Hardware: Arduino Nano, WS2812B LEDs, 2 potentiometers (A0, A1),
 *           LED data on pin 6, reset button on pin 2
 ****************************************************************************************/

#include <FastLED.h>
#include <stdlib.h>                 // For random() function

#define MATRIX_WIDTH        22
#define MATRIX_HEIGHT       10
#define NUM_LEDS            (MATRIX_WIDTH * MATRIX_HEIGHT)
#define LED_PIN             6       // Data pin for LED strip
#define PADDLE_HEIGHT       3       // Height of each paddle in LEDs
#define BALL_SPEED_MIN      40      // Fastest ball speed (ms)
#define BALL_SPEED_START    100     // Starting ball speed (ms)
#define POT1_PIN            A0      // Left player potentiometer
#define POT2_PIN            A1      // Right player potentiometer
#define SCOREBOARD_ENABLED  1       // 1 = scores on top row, 0 = Serial only
#define BOUNCE_ANGLES      1       // 1 = angled bounces, 0 = classic straight bounces
#define WINNING_SCORE       10      // Score required to win the match
#define FADE_AMOUNT         40      // Fade amount per frame for trails (0‑255)
#define RESET_BUTTON_PIN    2       // Pin for reset button (active‑low)

CRGB leds[NUM_LEDS];               // LED array

/* --------------------------------------------------------------
   Game state variables
   -------------------------------------------------------------- */
int   paddle1Y   = 0;       // Top Y position of left paddle
int   paddle2Y   = 0;       // Top Y position of right paddle
float smoothedPot1 = 512;   // Smoothed pot values – start in the middle
float smoothedPot2 = 512;
const float alpha = 0.7;    // Smoothing factor (higher = more responsive)

int   ballX      = MATRIX_WIDTH / 2;
int   ballY      = MATRIX_HEIGHT / 2;
int   ballDX     = 1;       // X direction: +1 right, -1 left
int   ballDY     = 1;       // Y direction: -1, 0, +1
int   score1     = 0;
int   score2     = 0;
int   ballSpeed  = BALL_SPEED_START;
unsigned long lastUpdate = 0;   // Timing for ball movement
int   rallyCount = 0;           // Consecutive paddle hits

/* --------------------------------------------------------------
   Reset‑button debouncing variables
   -------------------------------------------------------------- */
bool   lastResetButtonState = HIGH;
bool   resetButtonState     = HIGH;
unsigned long lastResetDebounceTime = 0;
const unsigned long resetDebounceDelay = 50;

/* --------------------------------------------------------------
   Forward declarations
   -------------------------------------------------------------- */
void resetBall(bool withAnimation);

/* --------------------------------------------------------------
   Helper: translate (x,y) to LED index for serpentine wiring
   -------------------------------------------------------------- */
int getLEDIndex(int x, int y) {
  if (y % 2 == 0) return y * MATRIX_WIDTH + x;
  return y * MATRIX_WIDTH + (MATRIX_WIDTH - 1 - x);
}

/* --------------------------------------------------------------
   Simple pixel setter that guards against out‑of‑bounds coordinates
   -------------------------------------------------------------- */
void setPixel(int x, int y, CRGB color) {
  if (x >= 0 && x < MATRIX_WIDTH && y >= 0 && y < MATRIX_HEIGHT)
    leds[getLEDIndex(x, y)] = color;
}

/* --------------------------------------------------------------
   Matrix clearing utility
   -------------------------------------------------------------- */
void clearMatrix() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
}

/* --------------------------------------------------------------
   *** NEW *** – flash only the paddles (used on scoring)
   -------------------------------------------------------------- */
void flashPaddles() {
  // Temporarily colour both paddles white
  for (int i = 0; i < PADDLE_HEIGHT; i++) {
    setPixel(0,                 paddle1Y + i, CRGB::White);                 // Left paddle
    setPixel(MATRIX_WIDTH - 1,  paddle2Y + i, CRGB::White);                 // Right paddle
  }
  FastLED.show();
  delay(50);                         // Visible flash duration
  // Normal paddle colours are restored in the next draw cycle
}

/* --------------------------------------------------------------
   *** DISABLED *** – full‑screen flash (kept for reference only)
   -------------------------------------------------------------- */
// void flashScreen() {
//   // Previously used to flash the entire matrix white on a score.
//   // Disabled because the game now flashes only the paddles.
//   // fill_solid(leds, NUM_LEDS, CRGB::White);
//   // FastLED.show();
//   // delay(50);
// }

/* --------------------------------------------------------------
   *** NEW *** – rainbow animation **only on the paddles**
   -------------------------------------------------------------- */
void rainbowPaddle() {
  // Animate a moving rainbow along the two paddle columns.
  // The hue is shifted each frame to give a flowing effect.
  uint8_t baseHue = beat8(10);               // Base hue drifts slowly
  const uint8_t hueStep = 16;                // Colour distance between successive LEDs

  // Draw left paddle
  for (int i = 0; i < PADDLE_HEIGHT; i++) {
    uint8_t h = baseHue + i * hueStep;
    setPixel(0, paddle1Y + i, CHSV(h, 255, 255));
  }
  // Draw right paddle
  for (int i = 0; i < PADDLE_HEIGHT; i++) {
    uint8_t h = baseHue + i * hueStep + 64;  // Offset so the two paddles aren’t identical
    setPixel(MATRIX_WIDTH - 1, paddle2Y + i, CHSV(h, 255, 255));
  }

  FastLED.show();
  delay(30);                                 // Speed of the rainbow “throw”
}

/* --------------------------------------------------------------
   Original full‑matrix rainbow wave (kept for reference only)
   -------------------------------------------------------------- */
// void rainbowWave() {
//   uint8_t initialHue = beat8(10);
//   for (int i = 0; i < MATRIX_WIDTH; i++) {
//     fill_rainbow(leds, NUM_LEDS, initialHue + i * 4, 10);
//     FastLED.show();
//     delay(15);
//   }
// }

/* --------------------------------------------------------------
   Draw paddles – left (blue) and right (green)
   -------------------------------------------------------------- */
void drawPaddles() {
  for (int i = 0; i < PADDLE_HEIGHT; i++) {
    setPixel(0,                 paddle1Y + i, CRGB::Blue);   // Left paddle
    setPixel(MATRIX_WIDTH - 1,  paddle2Y + i, CRGB::Green);  // Right paddle
  }
}

/* --------------------------------------------------------------
   Draw the ball (red)
   -------------------------------------------------------------- */
void drawBall() {
  setPixel(ballX, ballY, CRGB::Red);
}

/* --------------------------------------------------------------
   Reset‑button handling with software debouncing
   -------------------------------------------------------------- */
bool checkResetButton() {
  int reading = digitalRead(RESET_BUTTON_PIN);
  unsigned long now = millis();

  if (reading != lastResetButtonState) {
    lastResetDebounceTime = now;                // Reset timer on change
  }

  if ((now - lastResetDebounceTime) > resetDebounceDelay) {
    if (reading != resetButtonState) {
      resetButtonState = reading;
      if (resetButtonState == LOW) {           // Button pressed (active low)
        lastResetButtonState = reading;
        return true;
      }
    }
  }
  lastResetButtonState = reading;
  return false;
}

/* --------------------------------------------------------------
   Optional top‑row scoreboard (enabled with SCOREBOARD_ENABLED)
   -------------------------------------------------------------- */
void drawScoreboard() {
#if SCOREBOARD_ENABLED
  // Player 1 score – columns 0‑9 (red tens, blue units)
  int p1_tens   = score1 / 10;
  int p1_units  = score1 % 10;
  for (int i = 0; i < p1_tens;  i++) setPixel(i,                 0, CRGB::Red);
  for (int i = 0; i < p1_units; i++) setPixel(p1_tens + i,      0, CRGB::Blue);

  // Player 2 score – columns 12‑21 (yellow tens, green units)
  int p2_tens   = score2 / 10;
  int p2_units  = score2 % 10;
  for (int i = 0; i < p2_tens;  i++) setPixel(12 + i,            0, CRGB::Yellow);
  for (int i = 0; i < p2_units; i++) setPixel(12 + p2_tens + i, 0, CRGB::Green);
#endif
}

/* --------------------------------------------------------------
   Ball movement, wall & paddle collisions, scoring
   -------------------------------------------------------------- */
void updateBall() {
  // ---- Move the ball -------------------------------------------------
  ballX += ballDX;
  ballY += ballDY;

  // Debug output (Serial only when scoreboard disabled)
#if !SCOREBOARD_ENABLED
  Serial.print("Ball: ("); Serial.print(ballX); Serial.print(", ");
  Serial.print(ballY); Serial.print(") Speed: "); Serial.println(ballSpeed);
#endif

  // ---- Wall collisions (top / bottom) --------------------------------
  int minY = SCOREBOARD_ENABLED ? 1 : 0;   // Keep top row free for scoreboard
  if (ballY < minY) {
    ballY = minY;
    ballDY = -ballDY;
  } else if (ballY >= MATRIX_HEIGHT) {
    ballY = MATRIX_HEIGHT - 1;
    ballDY = -ballDY;
  }

  // ---- Paddle collisions ------------------------------------------------
#if BOUNCE_ANGLES                         // Angled bounce version
  // ----- Left paddle ----------------------------------------------------
  if (ballX == 0 && ballY >= paddle1Y && ballY < paddle1Y + PADDLE_HEIGHT) {
    // Flash paddle white (visual feedback)
    for (int i = 0; i < PADDLE_HEIGHT; i++) setPixel(0, paddle1Y + i, CRGB::White);
    FastLED.show(); delay(10);

    ballDX = 1;                     // Reverse X direction
    ballX += ballDX;                // Move away from paddle

    // Determine new Y direction based on hit position
    int hitPos = ballY - paddle1Y;  // 0 = top, 1 = middle, 2 = bottom
    if (hitPos == 0)       ballDY = -1;
    else if (hitPos == 1)  ballDY = (random(0,10) < 2) ? 0 : (random(0,2) ? 1 : -1); // 20 % neutral
    else                   ballDY =  1;

    rallyCount++;
    if (rallyCount >= 4 && ballSpeed > BALL_SPEED_MIN) {
      ballSpeed -= 5;               // Speed up after a few successful hits
      rallyCount = 0;
    }
  }
  // ----- Right paddle ---------------------------------------------------
  else if (ballX == MATRIX_WIDTH - 1 &&
           ballY >= paddle2Y && ballY < paddle2Y + PADDLE_HEIGHT) {
    // Flash paddle white (visual feedback)
    for (int i = 0; i < PADDLE_HEIGHT; i++) setPixel(MATRIX_WIDTH - 1, paddle2Y + i, CRGB::White);
    FastLED.show(); delay(10);

    ballDX = -1;                    // Reverse X direction
    ballX += ballDX;                // Move away from paddle

    int hitPos = ballY - paddle2Y;
    if (hitPos == 0)       ballDY = -1;
    else if (hitPos == 1)  ballDY = (random(0,10) < 2) ? 0 : (random(0,2) ? 1 : -1);
    else                   ballDY =  1;

    rallyCount++;
    if (rallyCount >= 4 && ballSpeed > BALL_SPEED_MIN) {
      ballSpeed -= 5;
      rallyCount = 0;
    }
  }
#else                                      // Classic straight‑bounce version
  if (ballX == 0 && ballY >= paddle1Y && ballY < paddle1Y + PADDLE_HEIGHT) {
    for (int i = 0; i < PADDLE_HEIGHT; i++) setPixel(0, paddle1Y + i, CRGB::White);
    FastLED.show(); delay(10);
    ballDX = -ballDX;
    ballX += ballDX;
    rallyCount++;
    if (rallyCount >= 4 && ballSpeed > BALL_SPEED_MIN) { ballSpeed -= 5; rallyCount = 0; }
  } else if (ballX == MATRIX_WIDTH - 1 &&
             ballY >= paddle2Y && ballY < paddle2Y + PADDLE_HEIGHT) {
    for (int i = 0; i < PADDLE_HEIGHT; i++) setPixel(MATRIX_WIDTH - 1, paddle2Y + i, CRGB::White);
    FastLED.show(); delay(10);
    ballDX = -ballDX;
    ballX += ballDX;
    rallyCount++;
    if (rallyCount >= 4 && ballSpeed > BALL_SPEED_MIN) { ballSpeed -= 5; rallyCount = 0; }
  }
#endif

  // ---- Scoring ---------------------------------------------------------
  if (ballX < 0) {                     // Right player scores
    score2++;
    rallyCount = 0;
#if !SCOREBOARD_ENABLED
    Serial.print("Score: P1 "); Serial.print(score1); Serial.print(" - P2 "); Serial.println(score2);
#endif
    flashPaddles();                   // <‑‑ NEW: paddle‑only flash
    resetBall(true);
  } else if (ballX >= MATRIX_WIDTH) { // Left player scores
    score1++;
    rallyCount = 0;
#if !SCOREBOARD_ENABLED
    Serial.print("Score: P1 "); Serial.print(score1); Serial.print(" - P2 "); Serial.println(score2);
#endif
    flashPaddles();                   // <‑‑ NEW: paddle‑only flash
    resetBall(true);
  }
}

/* --------------------------------------------------------------
   Reset ball to centre with random direction – optional animation
   -------------------------------------------------------------- */
void resetBall(bool withAnimation) {
  if (withAnimation) {
    // Previously we called rainbowWave() which painted the whole matrix.
    // Now we call rainbowPaddle() which paints a moving rainbow **only on the paddles**.
    rainbowPaddle();
  }

  ballX = MATRIX_WIDTH / 2;
  ballY = random(SCOREBOARD_ENABLED ? 1 : 0, MATRIX_HEIGHT);
  ballDX = random(0, 2) == 0 ? -1 : 1;

  // Ensure we don't start with a neutral vertical velocity
  do {
    ballDY = random(-1, 2);          // -1, 0, or +1
  } while (ballDY == 0);

  ballSpeed = BALL_SPEED_START;
  rallyCount = 0;
}

/* --------------------------------------------------------------
   Arduino setup
   -------------------------------------------------------------- */
void setup() {
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(30);
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  Serial.begin(115200);
  randomSeed(analogRead(A3));        // Use floating pin as seed
  resetBall(false);
}

/* --------------------------------------------------------------
   Main loop – reads pots, moves ball, checks win/reset, draws frame
   -------------------------------------------------------------- */
void loop() {
  // ----- Reset button ---------------------------------------------------
  if (checkResetButton()) {
    // Immediate game reset
    score1 = 0;
    score2 = 0;
    ballSpeed = BALL_SPEED_START;
    FastLED.setBrightness(30);

    // Quick visual cue for a manual reset
    fill_solid(leds, NUM_LEDS, CRGB::Purple);
    FastLED.show(); delay(100);
    FastLED.clear(); FastLED.show(); delay(100);

    resetBall(false);

#if !SCOREBOARD_ENABLED
    Serial.println("Game Reset! Score: P1 0 - P2 0");
#endif
  }

  // ----- Win condition --------------------------------------------------
  if (score1 >= WINNING_SCORE || score2 >= WINNING_SCORE) {
    FastLED.clear();
    CRGB winColor = (score1 >= WINNING_SCORE) ? CRGB::Blue : CRGB::Green;
    for (int i = 0; i < 5; i++) {
      fill_solid(leds, NUM_LEDS, winColor);
      FastLED.show(); delay(200);
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      FastLED.show(); delay(200);
    }
    score1 = 0;
    score2 = 0;
    resetBall(false);
  }

  // ----- Read potentiometers (with dummy reads for ADC stability) -----
  analogRead(POT1_PIN);                     // Dummy read
  int pot1 = analogRead(POT1_PIN);
  analogRead(POT2_PIN);                     // Dummy read
  int pot2 = analogRead(POT2_PIN);

  paddle1Y = map(pot1, 0, 1023,
                 SCOREBOARD_ENABLED ? 1 : 0,
                 MATRIX_HEIGHT - PADDLE_HEIGHT);
  paddle2Y = map(pot2, 0, 1023,
                 SCOREBOARD_ENABLED ? 1 : 0,
                 MATRIX_HEIGHT - PADDLE_HEIGHT);

  // ----- Ball timing ----------------------------------------------------
  if (millis() - lastUpdate > ballSpeed) {
    updateBall();
    lastUpdate = millis();
  }

  // ----- Brightness scales with total points (capped for safety) ------
  FastLED.setBrightness(min(30 + (score1 + score2) * 5, 100));

  // ----- Render frame ----------------------------------------------------
  fadeToBlackBy(leds, NUM_LEDS, FADE_AMOUNT);
  drawPaddles();
  drawBall();
  drawScoreboard();
  FastLED.show();
}
