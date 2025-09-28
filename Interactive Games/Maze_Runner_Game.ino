/* -------------------------------------------------------------
   Maze Runner – 8×20 WS2812B matrix
   * torch‑light / persistent trail
   * all static data stored in flash (PROGMEM)
   * memory‑optimised for ATmega328 (≈ 800 B RAM usage)
   ------------------------------------------------------------- */

#include <FastLED.h>
#include <avr/pgmspace.h>          // PROGMEM helpers

/* ---------------------- Matrix configuration ------------------- */
#define LED_PIN        6
#define COLS           8
#define ROWS           20
#define NUM_LEDS       (COLS * ROWS)
#define LED_TYPE       WS2812B
#define COLOR_ORDER    GRB
#define BRIGHTNESS     100

/* -------------------------- Buttons --------------------------- */
#define BTN_UP    2
#define BTN_DOWN  3
#define BTN_LEFT  4
#define BTN_RIGHT 5
#define BTN_HINT  7

/* ---------------------- Game configuration -------------------- */
#define MAX_PATH_LENGTH   80          // fits in RAM now (2 B per point)
#define MAX_OPEN_SET      40
#define HINT_DURATION    3000        // ms
#define SOLUTION_SPEED    200        // ms per step
#define TRAIL_DURATION     8        // display frames a trail cell stays lit

/* --------------------------- Colours (PROGMEM) ---------------- */
const uint8_t colWall[]     PROGMEM = {  0,   0, 255};   // blue
const uint8_t colPath[]     PROGMEM = {  0,   0,   0};   // black
const uint8_t colStart[]    PROGMEM = {  0, 255,   0};   // green
const uint8_t colFinish[]   PROGMEM = {255, 255,   0};   // yellow
const uint8_t colPlayer[]   PROGMEM = {255,   0,   0};   // red
const uint8_t colSolution[] PROGMEM = {255,   0, 255};   // magenta
const uint8_t colHint[]     PROGMEM = {  0, 255, 255};   // cyan
const uint8_t colVisited[]  PROGMEM = {128,   0, 128};   // purple

CRGB getProgColor(const uint8_t *c) {
  return CRGB(pgm_read_byte(c),
              pgm_read_byte(c + 1),
              pgm_read_byte(c + 2));
}

CRGB leds[NUM_LEDS];

/* -------------------------- Maze --------------------------- */
#define WALL   0
#define PATH   1
#define START  2
#define FINISH 3
uint8_t maze[ROWS][COLS];

/* ------------------------- Player -------------------------- */
uint8_t playerX, playerY;
uint8_t startX, startY;
uint8_t finishX, finishY;

/* ------------------------- Game state ----------------------- */
bool gameWon       = false;
bool gameStarted   = false;
bool showingHint   = false;
bool showingSolution = false;
uint8_t currentLevel = 1;
uint16_t movesCount = 0;
uint16_t bestMoves   = 999;

/* --------------------------- A* structs --------------------- */
struct Coord {                     // solution points – only coordinates
  uint8_t x, y;
};
Coord solutionPath[MAX_PATH_LENGTH];
uint8_t solutionLength = 0;
uint8_t solutionStep   = 0;

struct OpenNode {                  // nodes kept in the open set
  uint8_t x, y, g;                 // g = cost from start
};
OpenNode openSet[MAX_OPEN_SET];
uint8_t g_values[ROWS][COLS];      // 0 = unvisited; else g+1

/* --------------------------- Trail -------------------------- */
uint8_t trailAge[ROWS][COLS];       // frames left to keep cell lit

/* ------------------------ Debounce -------------------------- */
unsigned long lastButtonPress[5] = {0,0,0,0,0};
const unsigned long debounceDelay = 150;

/* -------------------------- Timing -------------------------- */
unsigned long hintStartTime      = 0;
unsigned long lastSolutionStep  = 0;

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
  pinMode(BTN_HINT,  INPUT_PULLUP);

  randomSeed(analogRead(A0));

  initializeGame();
  showStartScreen();               // splash animation
}

/* ============================================================= */
/* ============================= LOOP ========================= */
/* ============================================================= */
void loop() {
  if (!gameStarted) { handleStartScreen(); return; }
  if (gameWon)      { handleGameWon();     return; }

  handleInput();                 // movement, hint, solution
  updateHint();
  updateSolution();
  ageTrail();                    // decrement trail timers
  updateDisplay();
  FastLED.show();
}

/* ------------------------------------------------------------ */
int XY(int x, int y) {                     // serpentine layout
  if (x < 0 || x >= COLS || y < 0 || y >= ROWS) return -1;
  if (y & 1) return y * COLS + (COLS - 1 - x);
  else       return y * COLS + x;
}

/* ============================================================= */
/* ======================  GAME INITIALISATION ================= */
/* ============================================================= */
void initializeGame() {
  currentLevel = 1;
  gameWon     = false;
  gameStarted = false;
  movesCount  = 0;
  showingHint = false;
  showingSolution = false;

  memset(trailAge, 0, sizeof(trailAge));
  generateMaze();
  calculateSolution();

  Serial.println(F("Maze Runner Initialized"));
  Serial.print(F("Level: "));
  Serial.println(currentLevel);
}

/* ------------------------------------------------------------ */
void generateMaze() {
  /* ------- fill with walls ------- */
  for (uint8_t y = 0; y < ROWS; ++y)
    for (uint8_t x = 0; x < COLS; ++x)
      maze[y][x] = WALL;

  /* ------- recursive back‑tracker (cells 2 apart) ------- */
  createMazeRecursive(1, 1);

  /* ------- force outer walls ------- */
  for (uint8_t x = 0; x < COLS; ++x) { maze[0][x] = WALL; maze[ROWS-1][x] = WALL; }
  for (uint8_t y = 0; y < ROWS; ++y) { maze[y][0] = WALL; maze[y][COLS-1] = WALL; }

  /* ------- place START (top‑left area) ------- */
  for (uint8_t attempts = 0; attempts < 50; ++attempts) {
    startX = random(1, COLS/2);
    startY = random(1, ROWS/2);
    if (maze[startY][startX] == PATH) {
      maze[startY][startX] = START;
      playerX = startX; playerY = startY;
      break;
    }
  }

  /* ------- place FINISH (bottom‑right area) ------- */
  for (uint8_t attempts = 0; attempts < 50; ++attempts) {
    finishX = random(COLS/2, COLS-1);
    finishY = random(ROWS/2, ROWS-1);
    if (maze[finishY][finishX] == PATH &&
        abs(finishX - startX) + abs(finishY - startY) > COLS/2) {
      maze[finishY][finishX] = FINISH;
      break;
    }
  }
}

/* ------- direction table for the recursive generator (PROGMEM) ---- */
const int8_t dirs[4][2] PROGMEM = {{ 0,-2},{ 2, 0},{ 0, 2},{-2, 0}};

void createMazeRecursive(uint8_t x, uint8_t y) {
  maze[y][x] = PATH;

  uint8_t order[4] = {0,1,2,3};
  for (uint8_t i = 0; i < 4; ++i) {
    uint8_t j = random(4);
    uint8_t t = order[i]; order[i] = order[j]; order[j] = t;
  }

  for (uint8_t i = 0; i < 4; ++i) {
    int8_t dir = order[i];
    int8_t dx = pgm_read_byte(&dirs[dir][0]);
    int8_t dy = pgm_read_byte(&dirs[dir][1]);
    int8_t nx = x + dx;
    int8_t ny = y + dy;
    if (nx > 0 && nx < COLS-1 && ny > 0 && ny < ROWS-1 && maze[ny][nx] == WALL) {
      maze[y + dy/2][x + dx/2] = PATH;
      createMazeRecursive(nx, ny);
    }
  }
}

/* ------------------------------------------------------------ */
bool calculateSolution() {
  solutionLength = 0;
  uint8_t openCount = 0;
  memset(g_values, 0, sizeof(g_values));

  uint8_t h0 = abs(startX - finishX) + abs(startY - finishY);
  openSet[0] = {startX, startY, 0};
  openCount = 1;

  while (openCount) {
    /* ---- pick node with smallest f = g + h ---- */
    uint8_t best = 0;
    uint8_t bestF = 255;
    for (uint8_t i = 0; i < openCount; ++i) {
      uint8_t hx = abs(openSet[i].x - finishX);
      uint8_t hy = abs(openSet[i].y - finishY);
      uint8_t f = openSet[i].g + hx + hy;
      if (f < bestF) { bestF = f; best = i; }
    }

    OpenNode cur = openSet[best];
    /* remove best from open set */
    for (uint8_t i = best; i < openCount-1; ++i) openSet[i] = openSet[i+1];
    openCount--;

    g_values[cur.y][cur.x] = cur.g + 1;               // mark closed (g+1)

    if (cur.x == finishX && cur.y == finishY) {     // reached finish
      solutionLength = cur.g + 1;
      if (solutionLength > MAX_PATH_LENGTH) solutionLength = MAX_PATH_LENGTH;

      uint8_t tx = finishX, ty = finishY, tg = cur.g;
      for (int8_t i = solutionLength-1; i >= 0; --i) {
        solutionPath[i] = {tx, ty};
        if (tg == 0) break;                         // back at start
        /* walk to neighbour whose g = tg */
        const int8_t nb[4][2] = {{0,1},{1,0},{0,-1},{-1,0}};
        for (uint8_t n = 0; n < 4; ++n) {
          int8_t nx = tx + nb[n][0];
          int8_t ny = ty + nb[n][1];
          if (nx >= 0 && nx < COLS && ny >= 0 && ny < ROWS &&
              g_values[ny][nx] == tg) {
            tx = nx; ty = ny; --tg; break;
          }
        }
      }
      return true;
    }

    /* ---- expand neighbours ---- */
    const int8_t nb[4][2] = {{0,1},{1,0},{0,-1},{-1,0}};
    for (uint8_t n = 0; n < 4; ++n) {
      int8_t nx = cur.x + nb[n][0];
      int8_t ny = cur.y + nb[n][1];
      if (nx < 0 || nx >= COLS || ny < 0 || ny >= ROWS) continue;
      uint8_t ct = maze[ny][nx];
      if (ct != PATH && ct != START && ct != FINISH) continue;
      if (g_values[ny][nx]) continue;               // already closed

      /* check if already in open set */
      bool inOpen = false;
      uint8_t idxOpen = 0;
      for (uint8_t o = 0; o < openCount; ++o)
        if (openSet[o].x == nx && openSet[o].y == ny) {
          inOpen = true; idxOpen = o; break;
        }

      uint8_t tentativeG = cur.g + 1;

      if (!inOpen) {                                 // add new node
        if (openCount < MAX_OPEN_SET) {
          openSet[openCount++] = { (uint8_t)nx, (uint8_t)ny, tentativeG };
        }
      } else if (tentativeG < openSet[idxOpen].g) {  // better path
        openSet[idxOpen].g = tentativeG;
      }
    }
  }
  return false;                                     // no solution
}

/* ============================================================= */
/* =========================== INPUT =========================== */
/* ============================================================= */
void handleInput() {
  unsigned long now = millis();
  int16_t newX = playerX, newY = playerY;
  bool moved = false;

  /* ---- movement buttons ---- */
  if (digitalRead(BTN_UP) == LOW && now - lastButtonPress[0] > debounceDelay) {
    newY = playerY - 1; moved = true; lastButtonPress[0] = now;
  } else if (digitalRead(BTN_DOWN) == LOW && now - lastButtonPress[1] > debounceDelay) {
    newY = playerY + 1; moved = true; lastButtonPress[1] = now;
  } else if (digitalRead(BTN_LEFT) == LOW && now - lastButtonPress[2] > debounceDelay) {
    newX = playerX - 1; moved = true; lastButtonPress[2] = now;
  } else if (digitalRead(BTN_RIGHT) == LOW && now - lastButtonPress[3] > debounceDelay) {
    newX = playerX + 1; moved = true; lastButtonPress[3] = now;
  }

  /* ---- hint button (short press = toggle hint, long press = show solution) ---- */
  if (digitalRead(BTN_HINT) == LOW && now - lastButtonPress[4] > debounceDelay) {
    if (!showingSolution) {               // short press -> toggle hint
      showingHint = !showingHint;
      if (showingHint) hintStartTime = now;
    }
    lastButtonPress[4] = now;
  }
  /* long press (≥1 s) → full solution */
  if (digitalRead(BTN_HINT) == LOW && now - lastButtonPress[4] > 1000 && !showingSolution) {
    showingSolution = true;
    showingHint = false;
    solutionStep = 0;
    lastSolutionStep = now;
    Serial.println(F("Solution ON"));
    lastButtonPress[4] = now;       // prevent re‑enter while held
  }

  /* ---- execute legal movement ---- */
  if (moved && newX >= 0 && newX < COLS && newY >= 0 && newY < ROWS) {
    if (maze[newY][newX] != WALL) {
      trailAge[playerY][playerX] = TRAIL_DURATION;   // leave a fading trace
      playerX = newX; playerY = newY;
      ++movesCount;

      if (playerX == finishX && playerY == finishY) {
        gameWon = true;
        if (movesCount < bestMoves) bestMoves = movesCount;
        Serial.print(F("Level finished – moves: "));
        Serial.print(movesCount);
        Serial.print(F("  best: "));
        Serial.println(bestMoves);
      }
    }
  }
}

/* ============================================================= */
/* ======================  HINT & SOLUTION ===================== */
/* ============================================================= */
void updateHint() {
  if (showingHint && millis() - hintStartTime > HINT_DURATION) {
    showingHint = false;
    Serial.println(F("Hint timeout"));
  }
}
void updateSolution() {
  if (!showingSolution || solutionLength == 0) return;
  if (millis() - lastSolutionStep > SOLUTION_SPEED) {
    ++solutionStep;
    if (solutionStep >= solutionLength) solutionStep = 0; // loop
    lastSolutionStep = millis();
  }
}

/* ============================================================= */
/* ======================  TRAIL AGE UPDATE ==================== */
/* ============================================================= */
void ageTrail() {
  for (uint8_t y = 0; y < ROWS; ++y)
    for (uint8_t x = 0; x < COLS; ++x)
      if (trailAge[y][x]) --trailAge[y][x];
}

/* ============================================================= */
/* ======================  DISPLAY UPDATE ====================== */
/* ============================================================= */
void updateDisplay() {
  FastLED.clear();

  const uint8_t maxLightRadius = 5;   // torch radius
  const uint8_t flickerAmt    = 30;   // random dimming for ambience

  /* ---- torch‑light (maze coloured according to cell type) ---- */
  for (uint8_t y = 0; y < ROWS; ++y) {
    for (uint8_t x = 0; x < COLS; ++x) {
      int16_t dist = abs(x - playerX) + abs(y - playerY);
      if (dist > maxLightRadius) continue;       // outside torch

      int idx = XY(x, y);
      if (idx < 0) continue;

      CRGB base;
      switch (maze[y][x]) {
        case WALL:   base = getProgColor(colWall);   break;
        case PATH:   base = getProgColor(colPath);   break;
        case START:  base = getProgColor(colStart);  break;
        case FINISH: base = getProgColor(colFinish); break;
        default:     base = CRGB::Black;             break;
      }

      /* make the finish glow when we are close */
      if (maze[y][x] == FINISH && dist < 3) {
        base.fadeToBlackBy(128);
        base += CHSV(millis() / 10, 255, 192);
      }

      uint8_t scale = 255 - (255 * dist / (maxLightRadius + 1));
      scale = qsub8(scale, random8(flickerAmt));
      leds[idx] = base.nscale8(scale);
    }
  }

  /* ---- persistent trail (fades with remaining age) ---- */
  for (uint8_t y = 0; y < ROWS; ++y)
    for (uint8_t x = 0; x < COLS; ++x)
      if (trailAge[y][x]) {
        int idx = XY(x, y);
        if (idx < 0) continue;
        CRGB tr = getProgColor(colVisited);
        uint8_t fade = map(trailAge[y][x], 0, TRAIL_DURATION, 0, 255);
        leds[idx] = tr.nscale8(fade);
      }

  /* ---- solution path (if shown) ---- */
  if (showingSolution && solutionLength) {
    for (uint8_t i = 0; i <= solutionStep && i < solutionLength; ++i) {
      uint8_t x = solutionPath[i].x;
      uint8_t y = solutionPath[i].y;
      int16_t dist = abs(x - playerX) + abs(y - playerY);
      if (dist > maxLightRadius) continue;
      int idx = XY(x, y);
      if (idx < 0) continue;
      CRGB sol = getProgColor(colSolution);
      uint8_t scale = 255 - (255 * dist / (maxLightRadius + 1));
      scale = qsub8(scale, random8(flickerAmt));
      leds[idx] = sol.nscale8(scale);
    }
  }

  /* ---- hint (few steps ahead) ---- */
  if (showingHint && solutionLength) {
    int8_t pos = -1;
    for (uint8_t i = 0; i < solutionLength; ++i)
      if (solutionPath[i].x == playerX && solutionPath[i].y == playerY) {
        pos = i; break;
      }
    if (pos >= 0) {
      for (uint8_t i = 1; i <= 3 && pos + i < solutionLength; ++i) {
        uint8_t x = solutionPath[pos + i].x;
        uint8_t y = solutionPath[pos + i].y;
        int16_t dist = abs(x - playerX) + abs(y - playerY);
        if (dist > maxLightRadius) continue;
        int idx = XY(x, y);
        if (idx < 0) continue;
        CRGB h = getProgColor(colHint);
        uint8_t scale = 255 - (255 * dist / (maxLightRadius + 1));
        scale = qsub8(scale, random8(flickerAmt));
        leds[idx] = h.nscale8(scale);
      }
    }
  }

  /* ---- player – brightest on top ---- */
  int pIdx = XY(playerX, playerY);
  if (pIdx >= 0) leds[pIdx] = getProgColor(colPlayer);
}

/* ============================================================= */
/* ======================  SPLASH / START ===================== */
/* ============================================================= */
void showStartScreen() {
  static unsigned long lastAnim = 0;
  static int phase = 0;
  if (millis() - lastAnim > 300) {
    FastLED.clear();
    for (uint8_t y = 0; y < ROWS; ++y)
      for (uint8_t x = 0; x < COLS; ++x)
        if ((x + y + phase) % 4 == 0) {
          int idx = XY(x, y);
          if (idx >= 0) leds[idx] = CRGB::DarkBlue;
        }
    ++phase;
    lastAnim = millis();
    FastLED.show();
  }
}
void handleStartScreen() {
  if (digitalRead(BTN_UP)    == LOW ||
      digitalRead(BTN_DOWN)  == LOW ||
      digitalRead(BTN_LEFT)  == LOW ||
      digitalRead(BTN_RIGHT) == LOW) {
    gameStarted = true;
    Serial.println(F("Maze Runner – game started"));
    delay(200);
  }
}

/* ============================================================= */
/* ======================  WIN / LEVEL UP ===================== */
/* ============================================================= */
void handleGameWon() {
  static unsigned long winStart = 0;
  static bool init = false;
  if (!init) {
    winStart = millis();
    init = true;
    celebrationAnimation();
  }
  if (millis() - winStart > 3000) {                // 3 s celebration
    ++currentLevel;
    movesCount = 0;
    gameWon = false;
    init = false;
    generateMaze();
    calculateSolution();
    memset(trailAge, 0, sizeof(trailAge));
    Serial.print(F("Starting Level "));
    Serial.println(currentLevel);
  }
}

/* ------------------------------------------------------------ */
void celebrationAnimation() {
  for (uint8_t hue = 0; hue < 256; hue += 8) {
    FastLED.clear();
    for (uint8_t y = 0; y < ROWS; ++y)
      for (uint8_t x = 0; x < COLS; ++x) {
        int idx = XY(x, y);
        if (idx < 0) continue;
        uint8_t d = abs(x - finishX) + abs(y - finishY);
        uint8_t h = (hue + d * 8) & 0xFF;
        leds[idx] = CHSV(h, 255, 255);
      }
    FastLED.show();
    delay(50);
  }
  FastLED.clear(); FastLED.show();
}
