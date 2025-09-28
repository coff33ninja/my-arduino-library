/* -------------------------------------------------------------
   Snake – 10×22 WS2812B matrix
   Fixed self‑collision bug + a few safety tweaks
   ------------------------------------------------------------- */

#include <FastLED.h>
#include <avr/pgmspace.h>          // for F() strings

/* ---------------------- Matrix configuration ------------------- */
#define LED_PIN        6
#define COLS           10
#define ROWS           22
#define NUM_LEDS       (COLS * ROWS)
#define LED_TYPE       WS2812B
#define COLOR_ORDER    GRB
#define BRIGHTNESS     80

/* -------------------------- Buttons --------------------------- */
#define BTN_UP    2
#define BTN_DOWN  3
#define BTN_LEFT  4
#define BTN_RIGHT 5

/* ---------------------- Potentiometers ------------------------ */
#define POT_X           A1          // left / right
#define POT_Y           A2          // up   / down
#define POT_CENTER      512
#define POT_DEADZONE    120          // distance from centre needed to act
#define POT_STABLE_CNT  4            // reads required before accepting

/* ---------------------- Game configuration -------------------- */
#define INITIAL_SPEED   300   // ms between moves
#define SPEED_INCREMENT 15    // speed up per food
#define MIN_SPEED       100   // fastest possible

/* --------------------------- Colours -------------------------- */
#define COLOR_SNAKE        CRGB::Green
#define COLOR_FOOD         CRGB::Red
#define COLOR_BACKGROUND   CRGB::Black
#define COLOR_SNAKE_HEAD   CRGB::Lime

CRGB leds[NUM_LEDS];

/* --------------------------- Types --------------------------- */
enum Direction { UP, DOWN, LEFT, RIGHT };
Direction currentDirection = RIGHT;
Direction nextDirection    = RIGHT;

struct Point { uint8_t x, y; };
Point snake[NUM_LEDS];           // max possible length = number of LEDs
int    snakeLength = 3;
Point  food;
int    score       = 0;
bool   gameOver    = false;
bool   gameStarted = false;

/* ---------------------------- Timing -------------------------- */
unsigned long lastMove   = 0;
unsigned long moveDelay  = INITIAL_SPEED;

/* ------------------------ Debounce -------------------------- */
unsigned long lastButtonPress[4] = {0, 0, 0, 0};
const unsigned long debounceDelay = 50;

/* --------------------- Pot Hysteresis ----------------------- */
int potXStable = 0, potYStable = 0;   // consecutive readings past dead‑zone
bool directionLocked = false;         // one turn per move only

/* ============================================================= */
/* ============================= SETUP ========================= */
/* ============================================================= */
void setup() {
  Serial.begin(9600);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();

  pinMode(BTN_UP,    INPUT_PULLUP);
  pinMode(BTN_DOWN,  INPUT_PULLUP);
  pinMode(BTN_LEFT,  INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  /* analog pins need no pinMode */

  randomSeed(analogRead(A0));

  initializeGame();
  showStartScreen();                 // draws the first frame
}

/* ============================================================= */
/* ============================= LOOP ========================= */
/* ============================================================= */
void loop() {
  if (!gameStarted) {                // splash screen
    handleStartScreen();
    return;
  }
  if (gameOver) {                    // game‑over animation
    handleGameOver();
    return;
  }

  handleInput();                     // buttons **or** pots set nextDirection

  if (millis() - lastMove >= moveDelay) {
    currentDirection = nextDirection;
    moveSnake();
    lastMove = millis();
    directionLocked = false;          // a new turn may be accepted now
  }

  updateDisplay();
  FastLED.show();
}

/* ------------------------------------------------------------ */
int XY(int x, int y) {                // serpentine layout
  if (x < 0 || x >= COLS || y < 0 || y >= ROWS) return -1;
  if (y & 1) return y * COLS + (COLS - 1 - x);
  else       return y * COLS + x;
}

/* ============================================================= */
/* ======================  GAME INITIALISATION ================= */
/* ============================================================= */
void initializeGame() {
  snakeLength = 3;
  currentDirection = RIGHT;
  nextDirection    = RIGHT;
  score = 0;
  gameOver    = false;
  gameStarted = false;
  moveDelay   = INITIAL_SPEED;
  directionLocked = false;
  potXStable = potYStable = 0;

  snake[0] = {2, ROWS / 2};   // head
  snake[1] = {1, ROWS / 2};   // body
  snake[2] = {0, ROWS / 2};   // tail

  generateFood();

  Serial.println(F("Snake Game Initialized"));
  Serial.println(F("Score: 0"));
}

/* ------------------------------------------------------------ */
void generateFood() {
  const int MAX_TRIES = 30;
  int tries = 0;
  bool ok = false;

  while (!ok && tries < MAX_TRIES) {
    food.x = random(COLS);
    food.y = random(ROWS);
    ok = true;
    for (int i = 0; i < snakeLength; ++i) {
      if (snake[i].x == food.x && snake[i].y == food.y) {
        ok = false;
        break;
      }
    }
    ++tries;
  }

  if (!ok) {                     // board practically full → win
    gameOver = true;
    Serial.println(F("You Win! (no free food cells)"));
  }
}

/* ============================================================= */
/* =========================== INPUT =========================== */
/* ============================================================= */
void handleInput() {
  unsigned long now = millis();

  /* ---------- 1) BUTTON INPUT – highest priority ------------- */
  bool buttonUsed = false;

  if (digitalRead(BTN_UP) == LOW && now - lastButtonPress[0] > debounceDelay) {
    if (currentDirection != DOWN && !directionLocked) {
      nextDirection = UP;
      directionLocked = true;
    }
    lastButtonPress[0] = now;
    buttonUsed = true;
  }
  if (digitalRead(BTN_DOWN) == LOW && now - lastButtonPress[1] > debounceDelay) {
    if (currentDirection != UP && !directionLocked) {
      nextDirection = DOWN;
      directionLocked = true;
    }
    lastButtonPress[1] = now;
    buttonUsed = true;
  }
  if (digitalRead(BTN_LEFT) == LOW && now - lastButtonPress[2] > debounceDelay) {
    if (currentDirection != RIGHT && !directionLocked) {
      nextDirection = LEFT;
      directionLocked = true;
    }
    lastButtonPress[2] = now;
    buttonUsed = true;
  }
  if (digitalRead(BTN_RIGHT) == LOW && now - lastButtonPress[3] > debounceDelay) {
    if (currentDirection != LEFT && !directionLocked) {
      nextDirection = RIGHT;
      directionLocked = true;
    }
    lastButtonPress[3] = now;
    buttonUsed = true;
  }

  /* ---------- 2) POTENTIOMETER INPUT (only if no button) --- */
  if (!buttonUsed) {
    int xVal = analogRead(POT_X);
    int yVal = analogRead(POT_Y);
    int dx   = xVal - POT_CENTER;
    int dy   = yVal - POT_CENTER;

    bool horiz = abs(dx) > abs(dy);   // which axis dominates?

    if (horiz) {
      if (dx >  POT_DEADZONE) {              // RIGHT
        ++potXStable;
        potYStable = 0;
        if (potXStable >= POT_STABLE_CNT && currentDirection != LEFT && !directionLocked) {
          nextDirection = RIGHT;
          directionLocked = true;
          potXStable = 0;
        }
      } else if (dx < -POT_DEADZONE) {       // LEFT
        ++potXStable;
        potYStable = 0;
        if (potXStable >= POT_STABLE_CNT && currentDirection != RIGHT && !directionLocked) {
          nextDirection = LEFT;
          directionLocked = true;
          potXStable = 0;
        }
      } else {
        potXStable = 0;
      }
    } else {                                 // vertical dominates
      if (dy >  POT_DEADZONE) {              // DOWN
        ++potYStable;
        potXStable = 0;
        if (potYStable >= POT_STABLE_CNT && currentDirection != UP && !directionLocked) {
          nextDirection = DOWN;
          directionLocked = true;
          potYStable = 0;
        }
      } else if (dy < -POT_DEADZONE) {       // UP
        ++potYStable;
        potXStable = 0;
        if (potYStable >= POT_STABLE_CNT && currentDirection != DOWN && !directionLocked) {
          nextDirection = UP;
          directionLocked = true;
          potYStable = 0;
        }
      } else {
        potYStable = 0;
      }
    }
  }
}

/* ============================================================= */
/* ========================== MOVEMENT ========================= */
/* ============================================================= */
void moveSnake() {
  /* ---- compute new head using signed ints (avoids wrap‑around) ---- */
  int nx = snake[0].x;
  int ny = snake[0].y;

  switch (currentDirection) {
    case UP:    ny--; break;
    case DOWN:  ny++; break;
    case LEFT:  nx--; break;
    case RIGHT: nx++; break;
  }

  /* ---- wall collision --------------------------------------- */
  if (nx < 0 || nx >= COLS || ny < 0 || ny >= ROWS) {
    gameOver = true;
    Serial.println(F("Game Over – Hit Wall!"));
    return;
  }

  Point newHead = {(uint8_t)nx, (uint8_t)ny};

  /* ---- food test – we need it before the self‑collision check
        because when we **eat** we keep the tail, therefore the tail
        must be considered part of the body for collision detection. ---- */
  bool ateFood = (newHead.x == food.x && newHead.y == food.y);

  /* ---- self‑collision --------------------------------------- */
  int checkLimit = ateFood ? snakeLength : snakeLength - 1; // ignore tail if we're not eating
  for (int i = 0; i < checkLimit; ++i) {
    if (snake[i].x == newHead.x && snake[i].y == newHead.y) {
      gameOver = true;
      Serial.println(F("Game Over – Hit Self!"));
      return;
    }
  }

  /* ---- food handling ---------------------------------------- */
  if (ateFood) {
    snakeLength = min(snakeLength + 1, NUM_LEDS);
    ++score;
    moveDelay = max(MIN_SPEED, (int)moveDelay - SPEED_INCREMENT);
    generateFood();

    Serial.print(F("Score: "));
    Serial.print(score);
    Serial.print(F(" | Speed: "));
    Serial.println(moveDelay);

    if (snakeLength >= NUM_LEDS) {        // board completely filled
      gameOver = true;
      Serial.println(F("You Win! Full board!"));
      return;
    }
  } else {
    /* shift body forward – tail disappears */
    for (int i = snakeLength - 1; i > 0; --i) {
      snake[i] = snake[i - 1];
    }
  }

  /* finally write the new head */
  snake[0] = newHead;
}

/* ============================================================= */
/* ========================== DISPLAY =========================== */
/* ============================================================= */
void updateDisplay() {
  FastLED.clear();

  /* body (index 1 … length‑1) */
  for (int i = 1; i < snakeLength; ++i) {
    int idx = XY(snake[i].x, snake[i].y);
    if (idx >= 0) leds[idx] = COLOR_SNAKE;
  }

  /* head – brighter */
  int headIdx = XY(snake[0].x, snake[0].y);
  if (headIdx >= 0) leds[headIdx] = COLOR_SNAKE_HEAD;

  /* food */
  int foodIdx = XY(food.x, food.y);
  if (foodIdx >= 0) leds[foodIdx] = COLOR_FOOD;
}

/* ============================================================= */
/* ====================  START‑SCREEN HANDLING ================= */
/* ============================================================= */
void showStartScreen() {
  static unsigned long lastAnim = 0;
  static bool state = false;

  if (millis() - lastAnim > 500) {
    FastLED.clear();
    if (state) {
      for (int i = 0; i < NUM_LEDS; i += 20) leds[i] = CRGB::Green;
    }
    state = !state;
    lastAnim = millis();
    FastLED.show();                 // needed – otherwise nothing appears
  }
}

/* ------------------------------------------------------------ */
void handleStartScreen() {
  /* any button press **or** a strong pot movement starts the game */
  bool potActive = (abs(analogRead(POT_X) - POT_CENTER) > POT_DEADZONE) ||
                   (abs(analogRead(POT_Y) - POT_CENTER) > POT_DEADZONE);

  if (digitalRead(BTN_UP)    == LOW ||
      digitalRead(BTN_DOWN)  == LOW ||
      digitalRead(BTN_LEFT)  == LOW ||
      digitalRead(BTN_RIGHT) == LOW ||
      potActive) {
    gameStarted = true;
    Serial.println(F("Game Started!"));
    delay(200);                    // simple debounce
  }
}

/* ============================================================= */
/* =====================  GAME‑OVER HANDLING ==================== */
/* ============================================================= */
void handleGameOver() {
  static unsigned long startTime = 0;
  static bool init = false;

  if (!init) {
    startTime = millis();
    init = true;
    gameOverAnimation();
  }

  if (millis() - startTime > 3000) {   // after 3 s → restart
    init = false;
    initializeGame();
    showStartScreen();
  }
}

/* ------------------------------------------------------------ */
void gameOverAnimation() {
  for (int i = 0; i < 3; ++i) {
    fill_solid(leds, NUM_LEDS, CRGB::Red);
    FastLED.show();
    delay(200);
    FastLED.clear();
    FastLED.show();
    delay(200);
  }

  int lit = min(score, NUM_LEDS);
  for (int i = 0; i < lit; ++i) leds[i] = CRGB::Blue;
  FastLED.show();
}
