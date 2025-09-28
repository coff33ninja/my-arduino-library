/* -------------------------------------------------------------
   Cellular Automata on a 10 × 22 WS2812B matrix
   ------------------------------------------------------------- */

#include <FastLED.h>
#include <avr/pgmspace.h>          // needed for PROGMEM helpers

/* ---------------------- Matrix configuration ------------------- */
#define LED_PIN        6
#define COLS           10
#define ROWS           22
#define NUM_LEDS       (COLS * ROWS)
#define LED_TYPE       WS2812B
#define COLOR_ORDER    GRB
#define BRIGHTNESS     80

/* -------------------------- Buttons --------------------------- */
#define BTN_NEXT_RULE  2
#define BTN_RESET      3
#define BTN_SPEED      4

/* -------------------------- Speed ---------------------------- */
#define SPEED_SLOW   500
#define SPEED_MEDIUM 200
#define SPEED_FAST   100
#define SPEED_ULTRA   50

/* -------------------------- Cell states ---------------------- */
#define DEAD    0
#define ALIVE   1
#define DYING   2   // Brian's Brain
#define EXCITED 3   // (not used yet)

CRGB leds[NUM_LEDS];

/* -------------------------- Rules ---------------------------- */
enum AutomataRule {
  GAME_OF_LIFE = 0,
  RULE_30      = 1,
  RULE_110     = 2,
  BRIANS_BRAIN = 3,
  SEEDS        = 4,
  LIFE_WITHOUT_DEATH = 5,
  MAZE         = 6,
  CORAL_GROWTH = 7,
  TOTAL_RULES  = 8
};

/* ----------------------- Rule names (PROGMEM) --------------- */
const char ruleName_0[] PROGMEM = "Conway's Game of Life";
const char ruleName_1[] PROGMEM = "Rule 30";
const char ruleName_2[] PROGMEM = "Rule 110";
const char ruleName_3[] PROGMEM = "Brian's Brain";
const char ruleName_4[] PROGMEM = "Seeds";
const char ruleName_5[] PROGMEM = "Life Without Death";
const char ruleName_6[] PROGMEM = "Maze";
const char ruleName_7[] PROGMEM = "Coral Growth";

const char * const ruleNamesPROGMEM[TOTAL_RULES] PROGMEM = {
  ruleName_0, ruleName_1, ruleName_2, ruleName_3,
  ruleName_4, ruleName_5, ruleName_6, ruleName_7
};

/* ----------------- Colour tables (PROGMEM) ------------------- */
/* Each colour is stored as three bytes: {R,G,B}                */
const uint8_t ruleColorsPROGMEM[TOTAL_RULES][4][3] PROGMEM = {
  // Game of Life
  {{0,0,0},   {255,255,255}, {255,0,0},   {0,0,255}},
  // Rule 30
  {{0,0,0},   {255,255,0},   {255,165,0}, {255,0,0}},
  // Rule 110
  {{0,0,0},   {0,255,0},     {0,255,0},   {255,255,255}},
  // Brian's Brain (index 2 = DYING)
  {{0,0,0},   {0,0,255},     {0,0,80},    {128,0,128}},
  // Seeds
  {{0,0,0},   {0,255,255},   {0,0,255},   {255,255,255}},
  // Life Without Death
  {{0,0,0},   {255,0,255},   {255,192,203},{255,255,255}},
  // Maze
  {{0,0,0},   {128,0,128},   {238,130,238},{255,255,255}},
  // Coral Growth
  {{0,0,0},   {255,165,0},   {255,0,0},   {255,255,0}}
};

/* ----------------------- Game state -------------------------- */
uint8_t currentGrid[ROWS][COLS];
uint8_t nextGrid[ROWS][COLS];
AutomataRule currentRule = GAME_OF_LIFE;
int generation = 0;
bool paused = false;

/* -------------------------- Speed ---------------------------- */
int speedLevel = 1;                         // 0=slow … 3=ultra
int updateSpeeds[4] = {SPEED_SLOW, SPEED_MEDIUM, SPEED_FAST, SPEED_ULTRA};
unsigned long lastUpdate = 0;

/* ---------------------- Button handling ---------------------- */
unsigned long lastButtonPress[3] = {0, 0, 0};
const unsigned long debounceDelay = 200;

/* ----------------------- Pattern struct ---------------------- */
struct Pattern {
  uint8_t w, h;
  const uint8_t *data;      // pointer to PROGMEM data
};

/* ----------------------- Patterns (PROGMEM) ----------------- */
const uint8_t gliderData[9] PROGMEM = {
  0,1,0,
  0,0,1,
  1,1,1
};

const uint8_t pulsarData[25] PROGMEM = {
  0,1,1,1,0,
  1,0,0,0,1,
  1,0,0,0,1,
  1,0,0,0,1,
  0,1,1,1,0
};

const uint8_t beaconData[16] PROGMEM = {
  1,1,0,0,
  1,0,0,0,
  0,0,0,1,
  0,0,1,1
};

const Pattern glider = {3,3, gliderData};
const Pattern pulsar = {5,5, pulsarData};
const Pattern beacon = {4,4, beaconData};

/* ============================================================= */
/* ======================  SETUP & LOOP  ======================== */
/* ============================================================= */

void setup() {
  Serial.begin(9600);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();

  pinMode(BTN_NEXT_RULE, INPUT_PULLUP);
  pinMode(BTN_RESET,      INPUT_PULLUP);
  pinMode(BTN_SPEED,      INPUT_PULLUP);

  randomSeed(analogRead(A0));

  initializeGrid();

  Serial.println(F("Cellular Automata Started"));
  Serial.print(F("Rule: "));
  Serial.println(readRuleName(currentRule));
}

/* ------------------------------------------------------------ */
void loop() {
  handleInput();

  if (!paused && (millis() - lastUpdate >= (unsigned long)updateSpeeds[speedLevel])) {
    updateAutomata();
    ++generation;
    lastUpdate = millis();
  }

  updateDisplay();
  FastLED.show();
}

/* ============================================================= */
/* ======================  HELPER FUNCTIONS  ==================== */
/* ============================================================= */

/* ---- XY conversion (serpentine) ------------------------------- */
int XY(int x, int y) {
  if (x < 0 || x >= COLS || y < 0 || y >= ROWS) return -1;
  if (y & 1)               // odd rows are reversed
    return y * COLS + (COLS - 1 - x);
  else
    return y * COLS + x;
}

/* ---- Read a rule name stored in PROGMEM ---------------------- */
String readRuleName(uint8_t idx) {
  const char *ptr = (const char *)pgm_read_word(&ruleNamesPROGMEM[idx]);
  char buffer[32];
  strcpy_P(buffer, ptr);          // copy from flash to RAM
  return String(buffer);
}

/* ---- Get a colour from PROGMEM -------------------------------- */
CRGB getRuleColour(uint8_t rule, uint8_t state) {
  uint8_t r = pgm_read_byte(&ruleColorsPROGMEM[rule][state][0]);
  uint8_t g = pgm_read_byte(&ruleColorsPROGMEM[rule][state][1]);
  uint8_t b = pgm_read_byte(&ruleColorsPROGMEM[rule][state][2]);
  return CRGB(r, g, b);
}

/* ---- Simple cell accessor (used by Rule30/110) --------------- */
static uint8_t getCell(int r, int c) {
  return currentGrid[r][c];
}

/* ---- Initialise the grid for the current rule ---------------- */
void initializeGrid() {
  generation = 0;

  for (int y = 0; y < ROWS; ++y)
    for (int x = 0; x < COLS; ++x) {
      currentGrid[y][x] = DEAD;
      nextGrid[y][x]    = DEAD;
    }

  switch (currentRule) {
    case GAME_OF_LIFE:          seedGameOfLife();        break;
    case RULE_30:
    case RULE_110:              seedElementaryCA();      break;
    case BRIANS_BRAIN:          seedBriansBrain();       break;
    case SEEDS:                 seedSeeds();             break;
    case LIFE_WITHOUT_DEATH:    seedLifeWithoutDeath();  break;
    case MAZE:                  seedMaze();              break;
    case CORAL_GROWTH:          seedCoralGrowth();       break;
  }
}

/* ---- Pattern placement (reads from PROGMEM) ------------------- */
void placePattern(const Pattern& p, int startX, int startY) {
  for (int y = 0; y < p.h; ++y)
    for (int x = 0; x < p.w; ++x) {
      uint8_t val = pgm_read_byte(&p.data[y * p.w + x]);
      int gx = startX + x;
      int gy = startY + y;
      if (gx >= 0 && gx < COLS && gy >= 0 && gy < ROWS)
        currentGrid[gy][gx] = val;
    }
}

/* ---- Button handling (with debounce) -------------------------- */
void handleInput() {
  unsigned long now = millis();

  /* Next‑rule button */
  if (digitalRead(BTN_NEXT_RULE) == LOW &&
      now - lastButtonPress[0] > debounceDelay) {
    currentRule = (AutomataRule)((currentRule + 1) % TOTAL_RULES);
    initializeGrid();
    Serial.print(F("Switched to: "));
    Serial.println(readRuleName(currentRule));
    lastButtonPress[0] = now;
  }

  /* Reset button – toggles pause / run */
  if (digitalRead(BTN_RESET) == LOW &&
      now - lastButtonPress[1] > debounceDelay) {
    paused = !paused;
    Serial.println(paused ? F("Paused") : F("Running"));
    lastButtonPress[1] = now;
  }

  /* Speed button */
  if (digitalRead(BTN_SPEED) == LOW &&
      now - lastButtonPress[2] > debounceDelay) {
    speedLevel = (speedLevel + 1) % 4;
    const char *speedNames[] = {"Slow", "Medium", "Fast", "Ultra"};
    Serial.print(F("Speed: "));
    Serial.println(speedNames[speedLevel]);
    lastButtonPress[2] = now;
  }
}

/* ============================================================= */
/* ====================  SEEDING FUNCTIONS ===================== */
/* ============================================================= */

void seedGameOfLife() {
  placePattern(glider, 1, 1);
  placePattern(beacon, COLS - 5, 1);
  placePattern(pulsar, COLS / 2 - 2, ROWS / 2 - 2);

  for (int i = 0; i < 20; ++i) {
    int x = random(COLS);
    int y = random(ROWS);
    currentGrid[y][x] = ALIVE;
  }
}

void seedElementaryCA() {
  currentGrid[0][COLS / 2] = ALIVE;
}

void seedBriansBrain() {
  for (int i = 0; i < 30; ++i) {
    int x = random(COLS);
    int y = random(ROWS);
    currentGrid[y][x] = random(2) ? EXCITED : ALIVE;
  }
}

void seedSeeds() {
  for (int i = 0; i < 15; ++i)
    currentGrid[random(ROWS)][random(COLS)] = ALIVE;
}

void seedLifeWithoutDeath() {
  for (int i = 0; i < 10; ++i)
    currentGrid[random(ROWS)][random(COLS)] = ALIVE;
}

void seedMaze() {
  for (int y = 0; y < ROWS; y += 2)
    for (int x = 0; x < COLS; x += 2)
      if (random(3) == 0) currentGrid[y][x] = ALIVE;
}

void seedCoralGrowth() {
  currentGrid[ROWS / 2][COLS / 2]     = ALIVE;
  currentGrid[ROWS / 2][COLS / 2 - 1] = ALIVE;
  currentGrid[ROWS / 2][COLS / 2 + 1] = ALIVE;
}

/* ============================================================= */
/* ======================  AUTOMATA UPDATE ===================== */
/* ============================================================= */

void updateAutomata() {
  switch (currentRule) {
    case GAME_OF_LIFE:          updateGameOfLife();        break;
    case RULE_30:               updateRule30();           break;
    case RULE_110:              updateRule110();          break;
    case BRIANS_BRAIN:          updateBriansBrain();      break;
    case SEEDS:                 updateSeeds();            break;
    case LIFE_WITHOUT_DEATH:    updateLifeWithoutDeath(); break;
    case MAZE:                  updateMaze();             break;
    case CORAL_GROWTH:          updateCoralGrowth();      break;
  }

  /* copy next → current (row‑by‑row memcpy, fast) */
  for (int y = 0; y < ROWS; ++y)
    memcpy(currentGrid[y], nextGrid[y], COLS);
}

/* ---- Game of Life -------------------------------------------- */
void updateGameOfLife() {
  for (int y = 0; y < ROWS; ++y)
    for (int x = 0; x < COLS; ++x) {
      int n = countNeighbors(x, y, ALIVE);
      if (currentGrid[y][x] == ALIVE)
        nextGrid[y][x] = (n == 2 || n == 3) ? ALIVE : DEAD;
      else
        nextGrid[y][x] = (n == 3) ? ALIVE : DEAD;
    }
}

/* ---- Rule 30 (elementary CA) -------------------------------- */
void updateRule30() {
  /* scroll everything down one row */
  for (int y = ROWS - 1; y > 0; --y)
    memcpy(currentGrid[y], currentGrid[y - 1], COLS);

  /* compute new top row from the former second row (now at y = 1) */
  for (int x = 0; x < COLS; ++x) {
    uint8_t left   = (x == 0)     ? 0 : getCell(1, x - 1);
    uint8_t center =                getCell(1, x);
    uint8_t right  = (x == COLS-1)? 0 : getCell(1, x + 1);
    uint8_t pattern = (left << 2) | (center << 1) | right;
    bool newState = (pattern == 1) || (pattern == 2) ||
                    (pattern == 3) || (pattern == 4);
    nextGrid[0][x] = newState ? ALIVE : DEAD;
  }

  /* copy the scrolled rows into nextGrid */
  for (int y = 1; y < ROWS; ++y)
    memcpy(nextGrid[y], currentGrid[y], COLS);
}

/* ---- Rule 110 (elementary CA) ------------------------------- */
void updateRule110() {
  for (int y = ROWS - 1; y > 0; --y)
    memcpy(currentGrid[y], currentGrid[y - 1], COLS);

  for (int x = 0; x < COLS; ++x) {
    uint8_t left   = (x == 0)     ? 0 : getCell(1, x - 1);
    uint8_t center =                getCell(1, x);
    uint8_t right  = (x == COLS-1)? 0 : getCell(1, x + 1);
    uint8_t pattern = (left << 2) | (center << 1) | right;
    bool newState = (pattern == 1) || (pattern == 2) ||
                    (pattern == 3) || (pattern == 5) ||
                    (pattern == 6);
    nextGrid[0][x] = newState ? ALIVE : DEAD;
  }

  for (int y = 1; y < ROWS; ++y)
    memcpy(nextGrid[y], currentGrid[y], COLS);
}

/* ---- Brian's Brain ------------------------------------------- */
void updateBriansBrain() {
  for (int y = 0; y < ROWS; ++y)
    for (int x = 0; x < COLS; ++x) {
      int n = countNeighbors(x, y, ALIVE);
      switch (currentGrid[y][x]) {
        case DEAD:
          nextGrid[y][x] = (n == 2) ? ALIVE : DEAD;
          break;
        case ALIVE:
          nextGrid[y][x] = DYING;
          break;
        case DYING:
          nextGrid[y][x] = DEAD;
          break;
        default:
          nextGrid[y][x] = DEAD;
      }
    }
}

/* ---- Seeds ------------------------------------------------- */
void updateSeeds() {
  for (int y = 0; y < ROWS; ++y)
    for (int x = 0; x < COLS; ++x) {
      int n = countNeighbors(x, y, ALIVE);
      if (currentGrid[y][x] == ALIVE)
        nextGrid[y][x] = DEAD;
      else
        nextGrid[y][x] = (n == 2) ? ALIVE : DEAD;
    }
}

/* ---- Life Without Death -------------------------------------- */
void updateLifeWithoutDeath() {
  for (int y = 0; y < ROWS; ++y)
    for (int x = 0; x < COLS; ++x) {
      int n = countNeighbors(x, y, ALIVE);
      if (currentGrid[y][x] == ALIVE)
        nextGrid[y][x] = ALIVE;
      else
        nextGrid[y][x] = (n == 3) ? ALIVE : DEAD;
    }
}

/* ---- Maze -------------------------------------------------- */
void updateMaze() {
  for (int y = 0; y < ROWS; ++y)
    for (int x = 0; x < COLS; ++x) {
      int n = countNeighbors(x, y, ALIVE);
      if (currentGrid[y][x] == ALIVE)
        nextGrid[y][x] = (n >= 1 && n <= 5) ? ALIVE : DEAD;
      else
        nextGrid[y][x] = (n == 3) ? ALIVE : DEAD;
    }
}

/* ---- Coral Growth ------------------------------------------- */
void updateCoralGrowth() {
  for (int y = 0; y < ROWS; ++y)
    for (int x = 0; x < COLS; ++x) {
      int n = countNeighbors(x, y, ALIVE);
      if (currentGrid[y][x] == ALIVE)
        nextGrid[y][x] = ALIVE;
      else
        nextGrid[y][x] = (n >= 3) ? ALIVE : DEAD;
    }
}

/* ============================================================= */
/* ======================  NEIGHBOR COUNT ===================== */
/* ============================================================= */
int countNeighbors(int x, int y, uint8_t state) {
  int cnt = 0;
  for (int dy = -1; dy <= 1; ++dy)
    for (int dx = -1; dx <= 1; ++dx) {
      if (dx == 0 && dy == 0) continue;
      int nx = x + dx;
      int ny = y + dy;

      if (nx < 0)      nx = COLS - 1;
      if (nx >= COLS)  nx = 0;
      if (ny < 0)      ny = ROWS - 1;
      if (ny >= ROWS)  ny = 0;

      if (currentGrid[ny][nx] == state) ++cnt;
    }
  return cnt;
}

/* ============================================================= */
/* ======================  DISPLAY UPDATE ====================== */
/* ============================================================= */

void updateDisplay() {
  // FastLED.clear();   // already cleared at start of each loop
  for (int y = 0; y < ROWS; ++y)
    for (int x = 0; x < COLS; ++x) {
      int i = XY(x, y);
      if (i >= 0) {
        uint8_t st = currentGrid[y][x];
        leds[i] = getRuleColour(currentRule, st);
      }
    }

  /* subtle flash each 100 generations – visual milestone */
  if (generation % 100 == 0 && generation > 0) {
    for (int i = 0; i < NUM_LEDS; ++i)
      leds[i] = leds[i] + CRGB(20, 20, 20);
  }
}
