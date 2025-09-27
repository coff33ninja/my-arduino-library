#include <FastLED.h>

// Matrix Configuration
#define LED_PIN 6
#define COLS 10
#define ROWS 22
#define NUM_LEDS (COLS * ROWS)
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS 80

// Button Pins
#define BTN_NEXT_RULE 2
#define BTN_RESET 3
#define BTN_SPEED 4

// Timing Configuration
#define SPEED_SLOW 500
#define SPEED_MEDIUM 200
#define SPEED_FAST 100
#define SPEED_ULTRA 50

// Cell States
#define DEAD 0
#define ALIVE 1
#define DYING 2  // For Brian's Brain
#define EXCITED 3 // For some rules

CRGB leds[NUM_LEDS];

// Cellular Automata Rules
enum AutomataRule {
  GAME_OF_LIFE = 0,
  RULE_30 = 1,
  RULE_110 = 2,
  BRIANS_BRAIN = 3,
  SEEDS = 4,
  LIFE_WITHOUT_DEATH = 5,
  MAZE = 6,
  CORAL_GROWTH = 7,
  TOTAL_RULES = 8
};

// Rule names for serial output
const char* ruleNames[TOTAL_RULES] = {
  "Conway's Game of Life",
  "Rule 30",
  "Rule 110", 
  "Brian's Brain",
  "Seeds",
  "Life Without Death",
  "Maze",
  "Coral Growth"
};

// Color schemes for different rules
CRGB ruleColors[TOTAL_RULES][4] = {
  {CRGB::Black, CRGB::White, CRGB::Red, CRGB::Blue},        // Game of Life
  {CRGB::Black, CRGB::Yellow, CRGB::Orange, CRGB::Red},     // Rule 30
  {CRGB::Black, CRGB::Green, CRGB::Lime, CRGB::White},      // Rule 110
  {CRGB::Black, CRGB::Blue, CRGB::Red, CRGB::Purple},       // Brian's Brain
  {CRGB::Black, CRGB::Cyan, CRGB::Blue, CRGB::White},       // Seeds
  {CRGB::Black, CRGB::Magenta, CRGB::Pink, CRGB::White},    // Life Without Death
  {CRGB::Black, CRGB::Purple, CRGB::Violet, CRGB::White},   // Maze
  {CRGB::Black, CRGB::Orange, CRGB::Red, CRGB::Yellow}      // Coral Growth
};

// Game state
uint8_t currentGrid[ROWS][COLS];
uint8_t nextGrid[ROWS][COLS];
AutomataRule currentRule = GAME_OF_LIFE;
int generation = 0;
bool paused = false;

// Speed control
int speedLevel = 1; // 0=slow, 1=medium, 2=fast, 3=ultra
int updateSpeeds[4] = {SPEED_SLOW, SPEED_MEDIUM, SPEED_FAST, SPEED_ULTRA};
unsigned long lastUpdate = 0;

// Button handling
unsigned long lastButtonPress[3] = {0, 0, 0};
const unsigned long debounceDelay = 200;

// Pattern templates for seeding
struct Pattern {
  int width, height;
  uint8_t data[8][8];
};

// Famous patterns
Pattern glider = {3, 3, {
  {0, 1, 0, 0, 0, 0, 0, 0},
  {0, 0, 1, 0, 0, 0, 0, 0},
  {1, 1, 1, 0, 0, 0, 0, 0}
}};

Pattern pulsar = {5, 5, {
  {0, 1, 1, 1, 0, 0, 0, 0},
  {1, 0, 0, 0, 1, 0, 0, 0},
  {1, 0, 0, 0, 1, 0, 0, 0},
  {1, 0, 0, 0, 1, 0, 0, 0},
  {0, 1, 1, 1, 0, 0, 0, 0}
}};

Pattern beacon = {4, 4, {
  {1, 1, 0, 0, 0, 0, 0, 0},
  {1, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 1, 0, 0, 0, 0},
  {0, 0, 1, 1, 0, 0, 0, 0}
}};

void setup() {
  Serial.begin(9600);
  
  // Initialize LED strip
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  
  // Initialize button pins
  pinMode(BTN_NEXT_RULE, INPUT_PULLUP);
  pinMode(BTN_RESET, INPUT_PULLUP);
  pinMode(BTN_SPEED, INPUT_PULLUP);
  
  // Initialize random seed
  randomSeed(analogRead(A0));
  
  initializeGrid();
  Serial.println("Cellular Automata Started");
  Serial.print("Rule: ");
  Serial.println(ruleNames[currentRule]);
}

void loop() {
  handleInput();
  
  if (!paused && millis() - lastUpdate >= updateSpeeds[speedLevel]) {
    updateAutomata();
    generation++;
    lastUpdate = millis();
  }
  
  updateDisplay();
  FastLED.show();
}

// Convert 2D coordinates to LED index (serpentine layout)
int XY(int x, int y) {
  if (x < 0 || x >= COLS || y < 0 || y >= ROWS) return -1;
  
  if (y % 2 == 0) {
    return y * COLS + x;
  } else {
    return y * COLS + (COLS - 1 - x);
  }
}

void initializeGrid() {
  generation = 0;
  
  // Clear grids
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      currentGrid[y][x] = DEAD;
      nextGrid[y][x] = DEAD;
    }
  }
  
  // Seed based on current rule
  switch (currentRule) {
    case GAME_OF_LIFE:
      seedGameOfLife();
      break;
    case RULE_30:
    case RULE_110:
      seedElementaryCA();
      break;
    case BRIANS_BRAIN:
      seedBriansBrain();
      break;
    case SEEDS:
      seedSeeds();
      break;
    case LIFE_WITHOUT_DEATH:
      seedLifeWithoutDeath();
      break;
    case MAZE:
      seedMaze();
      break;
    case CORAL_GROWTH:
      seedCoralGrowth();
      break;
  }
}

void seedGameOfLife() {
  // Place some famous patterns
  placePattern(glider, 1, 1);
  placePattern(beacon, COLS-5, 1);
  placePattern(pulsar, COLS/2-2, ROWS/2-2);
  
  // Add some random noise
  for (int i = 0; i < 20; i++) {
    int x = random(COLS);
    int y = random(ROWS);
    currentGrid[y][x] = ALIVE;
  }
}

void seedElementaryCA() {
  // Single cell in the middle of top row
  currentGrid[0][COLS/2] = ALIVE;
}

void seedBriansBrain() {
  // Random pattern with some excited cells
  for (int i = 0; i < 30; i++) {
    int x = random(COLS);
    int y = random(ROWS);
    currentGrid[y][x] = random(2) == 0 ? ALIVE : EXCITED;
  }
}

void seedSeeds() {
  // Sparse random seeding
  for (int i = 0; i < 15; i++) {
    int x = random(COLS);
    int y = random(ROWS);
    currentGrid[y][x] = ALIVE;
  }
}

void seedLifeWithoutDeath() {
  // Start with a few cells
  for (int i = 0; i < 10; i++) {
    int x = random(COLS);
    int y = random(ROWS);
    currentGrid[y][x] = ALIVE;
  }
}

void seedMaze() {
  // Random maze-like pattern
  for (int y = 0; y < ROWS; y += 2) {
    for (int x = 0; x < COLS; x += 2) {
      if (random(3) == 0) {
        currentGrid[y][x] = ALIVE;
      }
    }
  }
}

void seedCoralGrowth() {
  // Start with a few seed points
  currentGrid[ROWS/2][COLS/2] = ALIVE;
  currentGrid[ROWS/2][COLS/2-1] = ALIVE;
  currentGrid[ROWS/2][COLS/2+1] = ALIVE;
}

void placePattern(Pattern& pattern, int startX, int startY) {
  for (int y = 0; y < pattern.height; y++) {
    for (int x = 0; x < pattern.width; x++) {
      int gridX = startX + x;
      int gridY = startY + y;
      if (gridX >= 0 && gridX < COLS && gridY >= 0 && gridY < ROWS) {
        currentGrid[gridY][gridX] = pattern.data[y][x];
      }
    }
  }
}

void handleInput() {
  unsigned long currentTime = millis();
  
  // Next rule button
  if (digitalRead(BTN_NEXT_RULE) == LOW && 
      currentTime - lastButtonPress[0] > debounceDelay) {
    currentRule = (AutomataRule)((currentRule + 1) % TOTAL_RULES);
    initializeGrid();
    Serial.print("Switched to: ");
    Serial.println(ruleNames[currentRule]);
    lastButtonPress[0] = currentTime;
  }
  
  // Reset button
  if (digitalRead(BTN_RESET) == LOW && 
      currentTime - lastButtonPress[1] > debounceDelay) {
    initializeGrid();
    Serial.println("Grid reset");
    lastButtonPress[1] = currentTime;
  }
  
  // Speed button
  if (digitalRead(BTN_SPEED) == LOW && 
      currentTime - lastButtonPress[2] > debounceDelay) {
    speedLevel = (speedLevel + 1) % 4;
    const char* speedNames[] = {"Slow", "Medium", "Fast", "Ultra"};
    Serial.print("Speed: ");
    Serial.println(speedNames[speedLevel]);
    lastButtonPress[2] = currentTime;
  }
}

void updateAutomata() {
  switch (currentRule) {
    case GAME_OF_LIFE:
      updateGameOfLife();
      break;
    case RULE_30:
      updateRule30();
      break;
    case RULE_110:
      updateRule110();
      break;
    case BRIANS_BRAIN:
      updateBriansBrain();
      break;
    case SEEDS:
      updateSeeds();
      break;
    case LIFE_WITHOUT_DEATH:
      updateLifeWithoutDeath();
      break;
    case MAZE:
      updateMaze();
      break;
    case CORAL_GROWTH:
      updateCoralGrowth();
      break;
  }
  
  // Copy next grid to current grid
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      currentGrid[y][x] = nextGrid[y][x];
    }
  }
}

void updateGameOfLife() {
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      int neighbors = countNeighbors(x, y, ALIVE);
      
      if (currentGrid[y][x] == ALIVE) {
        // Live cell
        if (neighbors < 2 || neighbors > 3) {
          nextGrid[y][x] = DEAD;  // Dies
        } else {
          nextGrid[y][x] = ALIVE; // Survives
        }
      } else {
        // Dead cell
        if (neighbors == 3) {
          nextGrid[y][x] = ALIVE; // Birth
        } else {
          nextGrid[y][x] = DEAD;
        }
      }
    }
  }
}

void updateRule30() {
  // Elementary cellular automaton - process row by row from top
  for (int y = ROWS - 1; y > 0; y--) {
    for (int x = 0; x < COLS; x++) {
      currentGrid[y][x] = currentGrid[y-1][x];
    }
  }
  
  // Calculate new top row based on Rule 30
  for (int x = 0; x < COLS; x++) {
    int left = (x > 0) ? currentGrid[1][x-1] : 0;
    int center = currentGrid[1][x];
    int right = (x < COLS-1) ? currentGrid[1][x+1] : 0;
    
    int pattern = (left << 2) | (center << 1) | right;
    
    // Rule 30: 00011110 in binary
    bool newState = (pattern == 1) || (pattern == 2) || 
                    (pattern == 3) || (pattern == 4);
    
    nextGrid[0][x] = newState ? ALIVE : DEAD;
  }
  
  // Copy other rows
  for (int y = 1; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      nextGrid[y][x] = currentGrid[y][x];
    }
  }
}

void updateRule110() {
  // Elementary cellular automaton - Rule 110
  for (int y = ROWS - 1; y > 0; y--) {
    for (int x = 0; x < COLS; x++) {
      currentGrid[y][x] = currentGrid[y-1][x];
    }
  }
  
  for (int x = 0; x < COLS; x++) {
    int left = (x > 0) ? currentGrid[1][x-1] : 0;
    int center = currentGrid[1][x];
    int right = (x < COLS-1) ? currentGrid[1][x+1] : 0;
    
    int pattern = (left << 2) | (center << 1) | right;
    
    // Rule 110: 01101110 in binary
    bool newState = (pattern == 1) || (pattern == 2) || 
                    (pattern == 3) || (pattern == 5) || 
                    (pattern == 6);
    
    nextGrid[0][x] = newState ? ALIVE : DEAD;
  }
  
  for (int y = 1; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      nextGrid[y][x] = currentGrid[y][x];
    }
  }
}

void updateBriansBrain() {
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      int neighbors = countNeighbors(x, y, ALIVE);
      
      switch (currentGrid[y][x]) {
        case DEAD:
          if (neighbors == 2) {
            nextGrid[y][x] = ALIVE;  // Birth
          } else {
            nextGrid[y][x] = DEAD;
          }
          break;
        case ALIVE:
          nextGrid[y][x] = DYING;    // Always dies next generation
          break;
        case DYING:
          nextGrid[y][x] = DEAD;     // Becomes dead
          break;
      }
    }
  }
}

void updateSeeds() {
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      int neighbors = countNeighbors(x, y, ALIVE);
      
      if (currentGrid[y][x] == ALIVE) {
        nextGrid[y][x] = DEAD;  // All live cells die
      } else {
        if (neighbors == 2) {
          nextGrid[y][x] = ALIVE; // Birth with exactly 2 neighbors
        } else {
          nextGrid[y][x] = DEAD;
        }
      }
    }
  }
}

void updateLifeWithoutDeath() {
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      int neighbors = countNeighbors(x, y, ALIVE);
      
      if (currentGrid[y][x] == ALIVE) {
        nextGrid[y][x] = ALIVE;  // Never dies
      } else {
        if (neighbors == 3) {
          nextGrid[y][x] = ALIVE; // Birth
        } else {
          nextGrid[y][x] = DEAD;
        }
      }
    }
  }
}

void updateMaze() {
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      int neighbors = countNeighbors(x, y, ALIVE);
      
      if (currentGrid[y][x] == ALIVE) {
        if (neighbors >= 1 && neighbors <= 5) {
          nextGrid[y][x] = ALIVE;  // Survives
        } else {
          nextGrid[y][x] = DEAD;   // Dies
        }
      } else {
        if (neighbors == 3) {
          nextGrid[y][x] = ALIVE;  // Birth
        } else {
          nextGrid[y][x] = DEAD;
        }
      }
    }
  }
}

void updateCoralGrowth() {
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      int neighbors = countNeighbors(x, y, ALIVE);
      
      if (currentGrid[y][x] == ALIVE) {
        nextGrid[y][x] = ALIVE;  // Coral doesn't die
      } else {
        if (neighbors >= 3) {
          nextGrid[y][x] = ALIVE; // Grows when surrounded
        } else {
          nextGrid[y][x] = DEAD;
        }
      }
    }
  }
}

int countNeighbors(int x, int y, uint8_t state) {
  int count = 0;
  
  for (int dy = -1; dy <= 1; dy++) {
    for (int dx = -1; dx <= 1; dx++) {
      if (dx == 0 && dy == 0) continue; // Skip center cell
      
      int nx = x + dx;
      int ny = y + dy;
      
      // Wrap around edges for toroidal topology
      if (nx < 0) nx = COLS - 1;
      if (nx >= COLS) nx = 0;
      if (ny < 0) ny = ROWS - 1;
      if (ny >= ROWS) ny = 0;
      
      if (currentGrid[ny][nx] == state) {
        count++;
      }
    }
  }
  
  return count;
}

void updateDisplay() {
  FastLED.clear();
  
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      int ledIndex = XY(x, y);
      if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
        uint8_t cellState = currentGrid[y][x];
        if (cellState < 4) {
          leds[ledIndex] = ruleColors[currentRule][cellState];
        }
      }
    }
  }
  
  // Add some visual feedback every 100 generations
  if (generation % 100 == 0 && generation > 0) {
    // Brief flash to indicate milestone
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = leds[i] + CRGB(20, 20, 20);
    }
  }
}