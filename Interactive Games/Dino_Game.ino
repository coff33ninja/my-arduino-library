/* -------------------------------------------------------------
   Dino‑Runner – 8 × 22 WS2812B matrix
   * torch‑light background
   * faint star‑field
   * obstacles, jump / duck, lives, score
   * **all static data in PROGMEM**
   * RAM‑optimised (≈ 1 KB on an UNO)
   ------------------------------------------------------------- */

#include <FastLED.h>
#include <avr/pgmspace.h>

/* ---------------------- Matrix configuration ------------------- */
#define LED_PIN     6
#define COLS        22
#define ROWS        10
#define NUM_LEDS    (COLS * ROWS)
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS  32

CRGB leds[NUM_LEDS];                     // ≈ 660 B RAM

/* ---------------------- Geometry ----------------------------- */
#define DINO_X          3                     // fixed horizontal position
#define DINO_WIDTH      3
#define DINO_HEIGHT     5
#define GROUND_Y        9

/* ---------------------- Physics ----------------------------- */
#define MAX_JUMP_HEIGHT 3
#define JUMP_VELOCITY   2
#define GRAVITY         1

/* -------------------------- Colours (PROGMEM) ---------------- */
const uint8_t colDino[]    PROGMEM = { 50,205, 50};                      // Lime
const uint8_t colCactus[]  PROGMEM = { 34,139, 34};                      // ForestGreen
const uint8_t colGround[]  PROGMEM = {139, 69, 19};                      // SaddleBrown
const uint8_t colSky[]     PROGMEM = { 25, 25,112};                      // MidnightBlue
const uint8_t colStar[]    PROGMEM = {255,255,255};                      // White
const uint8_t colInvincible[] PROGMEM = {255,255,255};                  // flashing white

CRGB getProgColor(const uint8_t *p) {
  return CRGB(pgm_read_byte(p),
              pgm_read_byte(p+1),
              pgm_read_byte(p+2));
}

/* -------------------------- Maze ----------------------------- */
#define WALL   0
#define PATH   1
#define START  2
#define FINISH 3
uint8_t maze[ROWS][COLS];                // 220 B RAM

/* -------------------------- Dino frames (PROGMEM) ----------- */
const uint8_t dinoFrames[6][DINO_HEIGHT][DINO_WIDTH] PROGMEM = {
  // 0 – run 1
  {{1,1,0},{1,1,1},{1,1,0},{1,0,1},{1,0,0}},
  // 1 – run 2
  {{1,1,0},{1,1,1},{1,1,0},{1,1,0},{0,1,0}},
  // 2 – run 3
  {{1,1,0},{1,1,1},{1,1,0},{0,0,0},{1,0,1}},
  // 3 – run 4 (variation)
  {{1,1,0},{1,1,1},{1,1,0},{1,0,1},{0,1,1}},
  // 4 – jump
  {{1,1,0},{1,1,1},{1,1,0},{1,1,1},{0,0,0}},
  // 5 – duck
  {{0,0,0},{1,1,0},{1,1,1},{1,1,0},{1,0,1}}
};

/* -------------------------- Obstacles ------------------------ */
struct Obstacle {
  int8_t  x;            // signed – can become negative
  uint8_t height;
  uint8_t width;
  bool    active;
};
#define MAX_OBSTACLES 8
Obstacle obstacles[MAX_OBSTACLES];
uint8_t activeObstacles = 0;
unsigned long lastSpawn = 0;

/* -------------------------- Game state ----------------------- */
struct GameState {
  int8_t   dinoY;                 // vertical offset (0‑3)
  int8_t   jumpVelocity;
  bool     isJumping;
  bool     isDucking;
  uint8_t  currentFrame;          // 0‑5
  uint8_t  speed;                 // 1‑5
  uint16_t score;                 // fits comfortably
  unsigned long lastFrameSwitch;
  bool     gameOver;
  uint8_t  lives;
  bool     invincible;
  unsigned long invincibilityTimer;
} game;

/* -------------------------- Stars --------------------------- */
struct Star {
  int8_t   x, y;                 // screen coordinates
  CRGB     color;
  uint8_t  brightness;           // 0‑60 (very dim)
  unsigned long lastTwinkle;
  bool     visible;
};
#define NUM_STARS 4                // fewer stars → less RAM
Star stars[NUM_STARS];

/* -------------------------- Buttons -------------------------- */
#define BTN_JUMP 2
#define BTN_DUCK 3
const unsigned long debounceDelay = 50;
bool buttonPressed = false, lastButtonState = false;
unsigned long lastDebounceTime = 0;

/* -------------------------- XY mapping ---------------------- */
int XY(int x, int y) {
  if (x < 0 || x >= COLS || y < 0 || y >= ROWS) return -1;
  int realX = (COLS - 1) - x;
  if (y & 1) return y * COLS + (COLS - 1 - realX);
  else      return y * COLS + realX;
}

/* ============================================================= */
/* ============================= SETUP ========================= */
/* ============================================================= */
void setup() {
  Serial.begin(9600);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 1000);
  pinMode(BTN_JUMP, INPUT_PULLUP);
  pinMode(BTN_DUCK, INPUT_PULLUP);
  randomSeed(analogRead(A0));
  FastLED.clear(); FastLED.show();
  initializeGame();
  initializeStars();
  Serial.println(F("Dino‑Runner started"));
}

/* ============================================================= */
/* ============================= LOOP ========================= */
/* ============================================================= */
void loop() {
  unsigned long now = millis();

  /* ---- input handling (debounced) ---- */
  bool jumpReading = digitalRead(BTN_JUMP) == LOW;
  if (jumpReading != lastButtonState) lastDebounceTime = now;
  if ((now - lastDebounceTime) > debounceDelay) {
    if (jumpReading != buttonPressed) {
      buttonPressed = jumpReading;
      if (buttonPressed && !game.isJumping && !game.gameOver && !game.isDucking)
        startJump();
    }
  }
  lastButtonState = jumpReading;
  game.isDucking = digitalRead(BTN_DUCK) == LOW && !game.isJumping && !game.gameOver;

  /* ---- timing – frame‑rate depends on speed ---- */
  uint16_t targetFrame = max(50U, 150U - (game.speed * 10U));
  static unsigned long lastFrame = 0;
  if (now - lastFrame >= targetFrame) {
    if (!game.gameOver) {
      updatePhysics();
      moveObstacles();
      if (checkCollision()) handleCollision();
      if (now % 1000 == 0) {                 // simple difficulty curve
        if (game.speed < 5) ++game.speed;
      }
    }
    drawScene();
    FastLED.show();
    lastFrame = now;
  }

  /* ---- animation frame change (running / jumping / duck) ---- */
  if (now - game.lastFrameSwitch >= getAnimationDelay()) {
    if (game.isJumping)      game.currentFrame = 4;
    else if (game.isDucking) game.currentFrame = 5;
    else                     game.currentFrame = (game.currentFrame + 1) % 4;
    game.lastFrameSwitch = now;
  }

  /* ---- obstacle spawning ---- */
  if (!game.gameOver && now - lastSpawn > getSpawnDelay()) {
    spawnObstacle();
    lastSpawn = now;
  }

  /* ---- invincibility timer ---- */
  if (game.invincible && now - game.invincibilityTimer > 1000)
    game.invincible = false;

  /* ---- restart after game‑over (any button) ---- */
  if (game.gameOver && (digitalRead(BTN_JUMP) == LOW || now % 3000 < 100))
    initializeGame();
}

/* ============================================================= */
/* ====================  GAME INITIALISATION =================== */
/* ============================================================= */
void initializeGame() {
  game = {};                         // zero all members
  game.speed = 1;
  game.lives = 3;
  game.dinoY = 0;
  game.currentFrame = 0;
  activeObstacles = 0;
  for (uint8_t i = 0; i < MAX_OBSTACLES; ++i) obstacles[i].active = false;
  generateMaze();
}

/* -------------------------- Global start cell ------------------- */
uint8_t startX = 0;      // column of START tile
uint8_t startY = 0;      // row of START tile

void generateMaze() {
  for (uint8_t y = 0; y < ROWS; ++y)
    for (uint8_t x = 0; x < COLS; ++x) maze[y][x] = WALL;
  createMazeRecursive(1, 1);
  for (uint8_t x = 0; x < COLS; ++x) { maze[0][x] = WALL; maze[ROWS-1][x] = WALL; }
  for (uint8_t y = 0; y < ROWS; ++y) { maze[y][0] = WALL; maze[y][COLS-1] = WALL; }

  /* start / finish */
  startPosition();
  finishPosition();
}
void startPosition() {
  for (uint8_t a = 0; a < 30; ++a) {
    uint8_t x = random(1, COLS/2);
    uint8_t y = random(1, ROWS/2);
    if (maze[y][x] == PATH) {
      maze[y][x] = START;
      startX = x;               // <<< store the coordinates globally
      startY = y;
      break;
    }
  }
}
void finishPosition() {
  for (uint8_t a = 0; a < 30; ++a) {
    uint8_t x = random(COLS/2, COLS-1);
    uint8_t y = random(ROWS/2, ROWS-1);
    if (maze[y][x] == PATH &&
        abs(x - startX) + abs(y - startY) > COLS/2) {
      maze[y][x] = FINISH;
      break;
    }
  }
}

/* ---------------------------------------------------------------- */
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
    int8_t nx = x + dx, ny = y + dy;
    if (nx>0 && nx<COLS-1 && ny>0 && ny<ROWS-1 && maze[ny][nx]==WALL) {
      maze[y+dy/2][x+dx/2] = PATH;
      createMazeRecursive(nx, ny);
    }
  }
}

/* ============================================================= */
/* =========================== STARS =========================== */
/* ============================================================= */
void initializeStars() {
  for (uint8_t i = 0; i < NUM_STARS; ++i) {
    stars[i].visible = false;
    bool placed = false;
    for (uint8_t tries = 0; tries < 30 && !placed; ++tries) {
      int8_t x = random(2, COLS-2);
      int8_t y = random(1, GROUND_Y-1);
      // no overlap test – cheap for 4 stars
      stars[i].x = x; stars[i].y = y;
      stars[i].color = CRGB(random(180,255), random(180,255), random(200,255));
      stars[i].brightness = random(8,30);
      stars[i].lastTwinkle = millis() + random(0,5000);
      stars[i].visible = true;
      placed = true;
    }
  }
}

/* ------------------------------------------------------------ */
void drawStars() {
  unsigned long now = millis();
  for (uint8_t i = 0; i < NUM_STARS; ++i) {
    if (!stars[i].visible) continue;
    uint8_t phase = (now - stars[i].lastTwinkle) % 3000;
    uint8_t curBright = 0;
    if (phase < 200)      curBright = map(phase,0,200,0,stars[i].brightness);
    else if (phase < 400) curBright = stars[i].brightness * 2;
    else if (phase < 2200)curBright = map(phase,400,2200,stars[i].brightness*2,0);
    else                  curBright = 0;
    if (curBright > 5) {
      int idx = XY(stars[i].x, stars[i].y);
      if (idx >= 0) {
        CRGB c = stars[i].color;
        c.nscale8(curBright);
        leds[idx] = c;
      }
    }
    if (random(100) < 2) stars[i].lastTwinkle = now;
  }
}

/* ============================================================= */
/* =========================== GAME LOGIC ===================== */
/* ============================================================= */
void startJump() { if (!game.isJumping) { game.isJumping = true; game.jumpVelocity = JUMP_VELOCITY; } }
void updatePhysics() {
  if (game.isJumping) {
    game.dinoY += game.jumpVelocity;
    game.jumpVelocity -= GRAVITY;
    if (game.dinoY <= 0) { game.dinoY = 0; game.jumpVelocity = 0; game.isJumping = false; }
    if (game.dinoY > MAX_JUMP_HEIGHT) { game.dinoY = MAX_JUMP_HEIGHT; game.jumpVelocity = 0; }
  }
}

/* ------------------------------------------------------------ */
void spawnObstacle() {
  if (activeObstacles >= MAX_OBSTACLES) return;
  for (uint8_t i = 0; i < MAX_OBSTACLES; ++i) if (!obstacles[i].active) {
    obstacles[i].x = COLS - 1;
    obstacles[i].height = random(2, min(5, game.speed+2));
    obstacles[i].width  = random(1, min(3, game.speed+1));
    obstacles[i].active = true;
    ++activeObstacles;
    break;
  }
}

/* ------------------------------------------------------------ */
void moveObstacles() {
  for (uint8_t i = 0; i < MAX_OBSTACLES; ++i) if (obstacles[i].active) {
    obstacles[i].x -= game.speed;
    if (obstacles[i].x < -obstacles[i].width) {
      obstacles[i].active = false;
      if (activeObstacles) --activeObstacles;
    }
  }
}

/* ------------------------------------------------------------ */
bool checkCollision() {
  if (game.invincible) return false;

  /* Dino box */
  int8_t dLeft   = DINO_X;
  int8_t dRight  = DINO_X + DINO_WIDTH - 1;
  int8_t dBottom = GROUND_Y - game.dinoY;
  int8_t dHeight = game.isDucking ? DINO_HEIGHT-1 : DINO_HEIGHT;
  int8_t dTop    = dBottom - dHeight + 1;

  for (uint8_t i = 0; i < MAX_OBSTACLES; ++i) if (obstacles[i].active) {
    int8_t oLeft   = obstacles[i].x;
    int8_t oRight  = obstacles[i].x + obstacles[i].width - 1;
    int8_t oTop    = GROUND_Y - obstacles[i].height + 1;
    int8_t oBottom = GROUND_Y;

    // AABB overlap test
    if (dRight < oLeft || dLeft > oRight ||
        dBottom < oTop   || dTop   > oBottom) continue;

    // Jump clearance check – simple
    if (game.isJumping && dTop <= oBottom) return true;
    if (!game.isJumping && !game.isDucking)       return true;
    if (!game.isJumping && game.isDucking && obstacles[i].height > 2) return true;
  }
  return false;
}

/* ------------------------------------------------------------ */
void handleCollision() {
  --game.lives;
  game.invincible = true;
  game.invincibilityTimer = millis();
  if (game.lives == 0) {
    game.gameOver = true;
    Serial.print(F("Game Over – Score: ")); Serial.println(game.score);
  } else {
    // Remove the offending obstacle
    for (uint8_t i = 0; i < MAX_OBSTACLES; ++i)
      if (obstacles[i].active &&
          obstacles[i].x >= DINO_X - 2 && obstacles[i].x <= DINO_X + DINO_WIDTH + 2) {
        obstacles[i].active = false;
        if (activeObstacles) --activeObstacles;
        break;
      }
  }
}

/* ============================================================= */
/* =========================== DRAWING ======================= */
/* ============================================================= */
void drawScene() {
  // dimmed sky background
  CRGB sky = getProgColor(colSky); sky.nscale8(25);
  fill_solid(leds, NUM_LEDS, sky);

  drawStars();          // twinkling stars
  drawGround();         // textured ground line
  drawObstacles();      // cactus / rocks
  drawDino();           // animated hero
  drawUI();             // lives, speed, score bar
}

/* ------------------------------------------------------------ */
void drawGround() {
  CRGB base = getProgColor(colGround);
  for (uint8_t x = 0; x < COLS; ++x) {
    int idx = XY(x, GROUND_Y);
    if (idx < 0) continue;
    CRGB c = base;
    uint8_t texture = (x + (millis()/150)) % 6;
    if (texture < 2) c.nscale8(120);
    else if (texture < 4) c = CRGB(160, 82, 45);
    else c.nscale8(180);
    leds[idx] = c;
  }
}

/* ------------------------------------------------------------ */
void drawObstacles() {
  for (uint8_t i = 0; i < MAX_OBSTACLES; ++i) if (obstacles[i].active) {
    for (uint8_t dx = 0; dx < obstacles[i].width; ++dx) {
      for (uint8_t dy = 0; dy < obstacles[i].height; ++dy) {
        int8_t x = obstacles[i].x + dx;
        int8_t y = GROUND_Y - dy;
        int idx = XY(x, y);
        if (idx < 0) continue;
        CRGB c = getProgColor(colCactus);
        if (dy == obstacles[i].height-1) c = CRGB::Green;
        else if (dy == obstacles[i].height-2) c = CRGB(34,139,34);
        if (random(10) < 2) c.nscale8(80);
        leds[idx] = c;
      }
    }
  }
}

/* ------------------------------------------------------------ */
void drawDino() {
  CRGB d = getProgColor(colDino);
  if (game.invincible && (millis()/100)%2) d = getProgColor(colInvincible);
  else if (game.gameOver) d = CRGB::Red;

  int8_t topY = (GROUND_Y - DINO_HEIGHT + 1) - game.dinoY;
  for (uint8_t dy = 0; dy < DINO_HEIGHT; ++dy) {
    int8_t screenY = topY + dy;
    if (screenY < 0 || screenY >= ROWS) continue;
    for (uint8_t dx = 0; dx < DINO_WIDTH; ++dx) {
      uint8_t pix = pgm_read_byte(&dinoFrames[game.currentFrame][dy][dx]);
      if (pix) {
        int idx = XY(DINO_X + dx, screenY);
        if (idx >= 0) leds[idx] = d;
      }
    }
  }
}

/* ------------------------------------------------------------ */
void drawUI() {
  // lives (top‑right hearts)
  for (uint8_t i = 0; i < game.lives && i < 3; ++i) {
    int idx1 = XY(COLS-1 - i*2, 0);
    int idx2 = XY(COLS-2 - i*2, 0);
    if (idx1 >= 0) leds[idx1] = CRGB::Red;
    if (idx2 >= 0) leds[idx2] = CRGB::Red;
  }
  // speed bar (top‑left)
  for (uint8_t i = 0; i < game.speed && i < 5; ++i) {
    int idx = XY(i, 0);
    if (idx >= 0) {
      leds[idx] = CRGB::Blue;
      if (i >= 3) leds[idx].nscale8(200);
    }
  }
  // simplistic score bar (row 1)
  uint16_t bar = min(game.score / 100, (uint16_t)20);
  for (uint8_t i = 0; i < bar; ++i) {
    int idx = XY(i, 1);
    if (idx >= 0) {
      CRGB c = CRGB::Yellow;
      c.nscale8(100 + (20 - i));
      leds[idx] = c;
    }
  }
}

/* ------------------------------------------------------------ */
uint16_t getAnimationDelay() {                 // faster animation at higher speed
  return max(80U, 200U - (game.speed * 30U));
}
uint16_t getSpawnDelay() {                     // quicker spawns as speed rises
  return max(800UL, 2500UL - (game.speed * 300UL)) + random(0,1000);
}
/* ============================================================= */