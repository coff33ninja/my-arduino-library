#include <FastLED.h>
#include <avr/pgmspace.h>

// Matrix Configuration
#define LED_PIN 6
#define COLS 8
#define ROWS 20
#define NUM_LEDS (COLS * ROWS)
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS 100

// Button Pins
#define BTN_UP 2
#define BTN_DOWN 3
#define BTN_LEFT 4
#define BTN_RIGHT 5
#define BTN_HINT 7

// Game Configuration
#define MAX_PATH_LENGTH 80 // Reduced since maze is smaller
#define MAX_OPEN_SET 40
#define HINT_DURATION 3000  // milliseconds
#define SOLUTION_SPEED 200  // milliseconds per step

// Colors
#define COLOR_WALL CRGB::Blue
#define COLOR_PATH CRGB::Black
#define COLOR_PLAYER CRGB::Red
#define COLOR_START CRGB::Green
#define COLOR_FINISH CRGB::Yellow
#define COLOR_SOLUTION CRGB::Magenta
#define COLOR_HINT CRGB::Cyan
#define COLOR_VISITED CRGB::Purple

CRGB leds[NUM_LEDS];

// Maze cell types
enum CellType {
  WALL = 0,
  PATH = 1,
  START = 2,
  FINISH = 3
};

// Game state
CellType maze[ROWS][COLS];
int playerX, playerY;
int startX, startY;
int finishX, finishY;
bool gameWon = false;
bool gameStarted = false;
bool showingHint = false;
bool showingSolution = false;
int currentLevel = 1;
int movesCount = 0;
int bestMoves = 999;

// Pathfinding
struct Point {
  uint8_t x, y;
  uint8_t f, g, h;
};

Point solutionPath[MAX_PATH_LENGTH];
int solutionLength = 0;
int solutionStep = 0;

// A* data structures
Point openSet[MAX_OPEN_SET];
uint8_t g_values[ROWS][COLS]; // 0 = not visited, g+1 otherwise

// Button handling
unsigned long lastButtonPress[5] = {0, 0, 0, 0, 0};
const unsigned long debounceDelay = 150;

// Timing
unsigned long hintStartTime = 0;
unsigned long lastSolutionStep = 0;

void setup() {
  Serial.begin(9600);
  
  // Initialize LED strip
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  
  // Initialize button pins
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_HINT, INPUT_PULLUP);
  
  // Initialize random seed
  randomSeed(analogRead(A0));
  
  initializeGame();
  showStartScreen();
}

void loop() {
  if (!gameStarted) {
    handleStartScreen();
    return;
  }
  
  if (gameWon) {
    handleGameWon();
    return;
  }
  
  handleInput();
  updateHint();
  updateSolution();
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

void initializeGame() {
  currentLevel = 1;
  gameWon = false;
  gameStarted = false;
  movesCount = 0;
  showingHint = false;
  showingSolution = false;
  
  generateMaze();
  calculateSolution();
  
  Serial.println(F("Maze Runner Initialized"));
  Serial.print(F("Level: "));
  Serial.println(currentLevel);
  Serial.print(F("Solution exists: "));
  Serial.println(solutionLength > 0 ? F("Yes") : F("No"));
}

void generateMaze() {
  // Initialize with walls
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      maze[y][x] = WALL;
    }
  }
  
  // Create random maze using recursive backtracking
  createMazeRecursive(1, 1);
  
  // Ensure borders are walls
  for (int x = 0; x < COLS; x++) {
    maze[0][x] = WALL;
    maze[ROWS-1][x] = WALL;
  }
  for (int y = 0; y < ROWS; y++) {
    maze[y][0] = WALL;
    maze[y][COLS-1] = WALL;
  }
  
  // Place start position (top-left area)
  for (int attempts = 0; attempts < 50; attempts++) {
    startX = random(1, COLS/2);
    startY = random(1, ROWS/2);
    if (maze[startY][startX] == PATH) {
      maze[startY][startX] = START;
      playerX = startX;
      playerY = startY;
      break;
    }
  }
  
  // Place finish position (bottom-right area)
  for (int attempts = 0; attempts < 50; attempts++) {
    finishX = random(COLS/2, COLS-1);
    finishY = random(ROWS/2, ROWS-1);
    if (maze[finishY][finishX] == PATH && 
        abs(finishX - startX) + abs(finishY - startY) > COLS/2) {
      maze[finishY][finishX] = FINISH;
      break;
    }
  }
}

void createMazeRecursive(int x, int y) {
  maze[y][x] = PATH;
  
  // Directions: up, right, down, left
  const int dirs[4][2] PROGMEM = {{0, -2}, {2, 0}, {0, 2}, {-2, 0}};
  
  // Shuffle directions
  int order[4] = {0, 1, 2, 3};
  for (int i = 0; i < 4; i++) {
    int j = random(4);
    int temp = order[i];
    order[i] = order[j];
    order[j] = temp;
  }
  
  for (int i = 0; i < 4; i++) {
    int dir_idx = order[i];
    int dx = pgm_read_word(&dirs[dir_idx][0]);
    int dy = pgm_read_word(&dirs[dir_idx][1]);
    
    int nx = x + dx;
    int ny = y + dy;
    
    if (nx > 0 && nx < COLS-1 && ny > 0 && ny < ROWS-1 && maze[ny][nx] == WALL) {
      maze[y + dy/2][x + dx/2] = PATH;
      createMazeRecursive(nx, ny);
    }
  }
}

bool calculateSolution() {
  solutionLength = 0;
  int openCount = 0;
  
  memset(g_values, 0, sizeof(g_values));
  
  // Add start point to open set
  uint8_t h = abs(startX - finishX) + abs(startY - finishY);
  openSet[0] = {(uint8_t)startX, (uint8_t)startY, h, 0, h};
  openCount = 1;
  
  while (openCount > 0) {
    // Find point with lowest f score
    int currentIndex = 0;
    for (int i = 1; i < openCount; i++) {
      if (openSet[i].f < openSet[currentIndex].f) {
        currentIndex = i;
      }
    }
    
    Point current = openSet[currentIndex];
    
    // Remove current from open set
    for (int i = currentIndex; i < openCount - 1; i++) {
      openSet[i] = openSet[i + 1];
    }
    openCount--;
    
    // Add to closed set by recording its g value (cost)
    g_values[current.y][current.x] = current.g + 1; // Use g+1 to distinguish from 0 (not visited)
    
    // Check if we reached the finish
    if (current.x == finishX && current.y == finishY) {
      // Reconstruct path from g_values
      solutionLength = current.g + 1;
      if (solutionLength > MAX_PATH_LENGTH) {
        solutionLength = MAX_PATH_LENGTH;
      }

      uint8_t pathX = finishX;
      uint8_t pathY = finishY;
      uint8_t currentG = current.g + 1;

      for (int i = solutionLength - 1; i >= 0; i--) {
        solutionPath[i] = {pathX, pathY, 0, 0, 0};
        if (currentG <= 1) break; // At start node

        int neighbors[4][2] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
        bool foundParent = false;
        for (int j = 0; j < 4; j++) {
          int nx = pathX + neighbors[j][0];
          int ny = pathY + neighbors[j][1];
          if (nx >= 0 && nx < COLS && ny >= 0 && ny < ROWS && g_values[ny][nx] == currentG - 1) {
            pathX = nx;
            pathY = ny;
            currentG--;
            foundParent = true;
            break;
          }
        }
        if (!foundParent) { 
          solutionLength = 0;
          return false;
        }
      }
      return true;
    }
    
    // Check all neighbors
    int neighbors[4][2] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
    
    for (int i = 0; i < 4; i++) {
      int nx = current.x + neighbors[i][0];
      int ny = current.y + neighbors[i][1];
      
      if (nx >= 0 && nx < COLS && ny >= 0 && ny < ROWS &&
          (maze[ny][nx] == PATH || maze[ny][nx] == START || maze[ny][nx] == FINISH) &&
          g_values[ny][nx] == 0) { // Not in closed set
        
        uint8_t tentativeG = current.g + 1;
        
        // Check if this neighbor is already in open set
        bool inOpenSet = false;
        int openIndex = -1;
        for (int j = 0; j < openCount; j++) {
          if (openSet[j].x == nx && openSet[j].y == ny) {
            inOpenSet = true;
            openIndex = j;
            break;
          }
        }
        
        if (!inOpenSet) {
          if (openCount < MAX_OPEN_SET) {
            uint8_t h = abs(nx - finishX) + abs(ny - finishY);
            openSet[openCount] = {(uint8_t)nx, (uint8_t)ny, (uint8_t)(tentativeG + h), tentativeG, h};
            openCount++;
          }
        } else if (tentativeG < openSet[openIndex].g) {
          openSet[openIndex].g = tentativeG;
          openSet[openIndex].f = openSet[openIndex].g + openSet[openIndex].h;
        }
      }
    }
  }
  
  return false; // No solution found
}

void handleInput() {
  unsigned long currentTime = millis();
  
  int newX = playerX;
  int newY = playerY;
  bool moved = false;
  
  // Movement buttons
  if (digitalRead(BTN_UP) == LOW && currentTime - lastButtonPress[0] > debounceDelay) {
    newY = playerY - 1;
    moved = true;
    lastButtonPress[0] = currentTime;
  }
  else if (digitalRead(BTN_DOWN) == LOW && currentTime - lastButtonPress[1] > debounceDelay) {
    newY = playerY + 1;
    moved = true;
    lastButtonPress[1] = currentTime;
  }
  else if (digitalRead(BTN_LEFT) == LOW && currentTime - lastButtonPress[2] > debounceDelay) {
    newX = playerX - 1;
    moved = true;
    lastButtonPress[2] = currentTime;
  }
  else if (digitalRead(BTN_RIGHT) == LOW && currentTime - lastButtonPress[3] > debounceDelay) {
    newX = playerX + 1;
    moved = true;
    lastButtonPress[3] = currentTime;
  }
  
  // Hint button
  if (digitalRead(BTN_HINT) == LOW && currentTime - lastButtonPress[4] > debounceDelay) {
    if (!showingSolution) {
      showingHint = !showingHint;
      if (showingHint) {
        hintStartTime = currentTime;
        Serial.println(F("Showing hint..."));
      } else {
        Serial.println(F("Hint hidden"));
      }
    } else {
      showingSolution = false;
      Serial.println(F("Solution hidden"));
    }
    lastButtonPress[4] = currentTime;
  }
  
  // Long press hint button for full solution
  if (digitalRead(BTN_HINT) == LOW && currentTime - lastButtonPress[4] > 1000 && !showingSolution) {
    showingSolution = true;
    showingHint = false;
    solutionStep = 0;
    lastSolutionStep = currentTime;
    Serial.println(F("Showing solution..."));
    lastButtonPress[4] = currentTime;
  }
  
  // Validate and execute movement
  if (moved && newX >= 0 && newX < COLS && newY >= 0 && newY < ROWS) {
    if (maze[newY][newX] != WALL) {
      playerX = newX;
      playerY = newY;
      movesCount++;
      
      // Check win condition
      if (playerX == finishX && playerY == finishY) {
        gameWon = true;
        if (movesCount < bestMoves) {
          bestMoves = movesCount;
        }
        Serial.print(F("Level Complete! Moves: "));
        Serial.print(movesCount);
        Serial.print(F(" | Best: "));
        Serial.println(bestMoves);
      }
    }
  }
}

void updateHint() {
  if (showingHint && millis() - hintStartTime > HINT_DURATION) {
    showingHint = false;
    Serial.println(F("Hint expired"));
  }
}

void updateSolution() {
  if (!showingSolution || solutionLength == 0) return;
  
  if (millis() - lastSolutionStep > SOLUTION_SPEED) {
    solutionStep++;
    if (solutionStep >= solutionLength) {
      solutionStep = 0;  // Loop the solution
    }
    lastSolutionStep = millis();
  }
}

void updateDisplay() {
  FastLED.clear();
  
  const int maxLightRadius = 5;
  const int flickerAmount = 30;

  // Draw maze with torchlight effect
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      int distance = abs(x - playerX) + abs(y - playerY);

      if (distance <= maxLightRadius) {
        int ledIndex = XY(x, y);
        if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
          CRGB originalColor;
          switch (maze[y][x]) {
            case WALL:   originalColor = COLOR_WALL;   break;
            case PATH:   originalColor = COLOR_PATH;   break;
            case START:  originalColor = COLOR_START;  break;
            case FINISH: originalColor = COLOR_FINISH; break;
          }

          // Make the finish point glow when player is near
          if (maze[y][x] == FINISH && distance < 3) {
              originalColor.fadeToBlackBy(128);
              originalColor += CHSV(millis()/10, 255, 192);
          }

          uint8_t scale = 255 - (255 * distance / (maxLightRadius + 1));
          scale = qsub8(scale, random8(flickerAmount));
          
          leds[ledIndex] = originalColor.nscale8(scale);
        }
      }
    }
  }
  
  // Draw solution path (if active and visible)
  if (showingSolution && solutionLength > 0) {
    for (int i = 0; i <= solutionStep && i < solutionLength; i++) {
      int x = solutionPath[i].x;
      int y = solutionPath[i].y;
      int distance = abs(x - playerX) + abs(y - playerY);
      if (distance <= maxLightRadius) {
        int ledIndex = XY(x, y);
        if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
          uint8_t scale = 255 - (255 * distance / (maxLightRadius + 1));
          scale = qsub8(scale, random8(flickerAmount));
          CRGB solutionColor = COLOR_SOLUTION;
          leds[ledIndex] = solutionColor.nscale8(scale);
        }
      }
    }
  }
  
  // Draw hint (if active and visible)
  if (showingHint && solutionLength > 0) {
    int playerPosInSolution = -1;
    for (int i = 0; i < solutionLength; i++) {
      if (solutionPath[i].x == playerX && solutionPath[i].y == playerY) {
        playerPosInSolution = i;
        break;
      }
    }
    
    if (playerPosInSolution >= 0) {
      for (int i = 1; i <= 3 && playerPosInSolution + i < solutionLength; i++) {
        int x = solutionPath[playerPosInSolution + i].x;
        int y = solutionPath[playerPosInSolution + i].y;
        int distance = abs(x - playerX) + abs(y - playerY);
        if (distance <= maxLightRadius) {
            int ledIndex = XY(x, y);
            if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
                uint8_t scale = 255 - (255 * distance / (maxLightRadius + 1));
                scale = qsub8(scale, random8(flickerAmount));
                CRGB hintColor = COLOR_HINT;
                leds[ledIndex] = hintColor.nscale8(scale);
            }
        }
      }
    }
  }
  
  // Draw player (always on top and bright)
  int playerIndex = XY(playerX, playerY);
  if (playerIndex >= 0 && playerIndex < NUM_LEDS) {
    leds[playerIndex] = COLOR_PLAYER;
  }
}

void showStartScreen() {
  static unsigned long lastAnimation = 0;
  static int animationPhase = 0;
  
  if (millis() - lastAnimation > 300) {
    FastLED.clear();
    
    // Maze preview animation
    for (int y = 0; y < ROWS; y++) {
      for (int x = 0; x < COLS; x++) {
        if ((x + y + animationPhase) % 4 == 0) {
          int ledIndex = XY(x, y);
          if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
            leds[ledIndex] = CRGB::DarkBlue;
          }
        }
      }
    }
    
    animationPhase++;
    lastAnimation = millis();
    FastLED.show();
  }
}

void handleStartScreen() {
  // Any movement button starts the game
  if (digitalRead(BTN_UP) == LOW || digitalRead(BTN_DOWN) == LOW ||
      digitalRead(BTN_LEFT) == LOW || digitalRead(BTN_RIGHT) == LOW) {
    gameStarted = true;
    Serial.println(F("Maze Runner Started!"));
    delay(200);
  }
}

void handleGameWon() {
  static unsigned long celebrationStart = 0;
  static bool celebrationInitialized = false;
  
  if (!celebrationInitialized) {
    celebrationStart = millis();
    celebrationInitialized = true;
    celebrationAnimation();
  }
  
  // Show celebration for 3 seconds, then next level
  if (millis() - celebrationStart > 3000) {
    currentLevel++;
    movesCount = 0;
    gameWon = false;
    celebrationInitialized = false;
    
    // Generate new maze
    generateMaze();
    calculateSolution();
    
    Serial.print(F("Starting Level: "));
    Serial.println(currentLevel);
  }
}

void celebrationAnimation() {
  // Rainbow spiral from finish to start
  for (int i = 0; i < 256; i += 8) {
    FastLED.clear();
    
    for (int y = 0; y < ROWS; y++) {
      for (int x = 0; x < COLS; x++) {
        int ledIndex = XY(x, y);
        if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
          int distToFinish = abs(x - finishX) + abs(y - finishY);
          byte hue = (i + distToFinish * 8) % 256;
          leds[ledIndex] = CHSV(hue, 255, 255);
        }
      }
    }
    
    FastLED.show();
    delay(50);
  }
    FastLED.clear();
    FastLED.show();
}
