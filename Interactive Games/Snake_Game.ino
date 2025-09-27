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
#define BTN_UP 2
#define BTN_DOWN 3
#define BTN_LEFT 4
#define BTN_RIGHT 5

// Game Configuration
#define INITIAL_SPEED 300  // milliseconds between moves
#define SPEED_INCREMENT 15  // speed increase per food eaten
#define MIN_SPEED 100       // fastest possible speed

// Colors
#define COLOR_SNAKE CRGB::Green
#define COLOR_FOOD CRGB::Red
#define COLOR_BACKGROUND CRGB::Black
#define COLOR_SNAKE_HEAD CRGB::Lime

CRGB leds[NUM_LEDS];

// Game State
enum Direction { UP, DOWN, LEFT, RIGHT };
Direction currentDirection = RIGHT;
Direction nextDirection = RIGHT;

struct Point {
  int x, y;
};

Point snake[NUM_LEDS];  // Maximum possible snake length
int snakeLength = 3;
Point food;
int score = 0;
bool gameOver = false;
bool gameStarted = false;

// Timing
unsigned long lastMove = 0;
unsigned long moveDelay = INITIAL_SPEED;

// Button debouncing
unsigned long lastButtonPress[4] = {0, 0, 0, 0};
const unsigned long debounceDelay = 50;

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
  
  if (gameOver) {
    handleGameOver();
    return;
  }
  
  handleInput();
  
  if (millis() - lastMove >= moveDelay) {
    currentDirection = nextDirection;
    moveSnake();
    lastMove = millis();
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

void initializeGame() {
  snakeLength = 3;
  currentDirection = RIGHT;
  nextDirection = RIGHT;
  score = 0;
  gameOver = false;
  gameStarted = false;
  moveDelay = INITIAL_SPEED;
  
  // Initialize snake in center-left of matrix
  snake[0] = {2, ROWS/2};    // Head
  snake[1] = {1, ROWS/2};    // Body
  snake[2] = {0, ROWS/2};    // Tail
  
  generateFood();
  
  Serial.println("Snake Game Initialized");
  Serial.println("Score: 0");
}

void generateFood() {
  bool validPosition = false;
  
  while (!validPosition) {
    food.x = random(COLS);
    food.y = random(ROWS);
    
    // Check if food position conflicts with snake
    validPosition = true;
    for (int i = 0; i < snakeLength; i++) {
      if (snake[i].x == food.x && snake[i].y == food.y) {
        validPosition = false;
        break;
      }
    }
  }
}

void handleInput() {
  unsigned long currentTime = millis();
  
  // Up button
  if (digitalRead(BTN_UP) == LOW && currentTime - lastButtonPress[0] > debounceDelay) {
    if (currentDirection != DOWN) {  // Can't reverse directly
      nextDirection = UP;
    }
    lastButtonPress[0] = currentTime;
  }
  
  // Down button
  if (digitalRead(BTN_DOWN) == LOW && currentTime - lastButtonPress[1] > debounceDelay) {
    if (currentDirection != UP) {
      nextDirection = DOWN;
    }
    lastButtonPress[1] = currentTime;
  }
  
  // Left button
  if (digitalRead(BTN_LEFT) == LOW && currentTime - lastButtonPress[2] > debounceDelay) {
    if (currentDirection != RIGHT) {
      nextDirection = LEFT;
    }
    lastButtonPress[2] = currentTime;
  }
  
  // Right button
  if (digitalRead(BTN_RIGHT) == LOW && currentTime - lastButtonPress[3] > debounceDelay) {
    if (currentDirection != LEFT) {
      nextDirection = RIGHT;
    }
    lastButtonPress[3] = currentTime;
  }
}

void moveSnake() {
  // Calculate new head position
  Point newHead = snake[0];
  
  switch (currentDirection) {
    case UP:
      newHead.y--;
      break;
    case DOWN:
      newHead.y++;
      break;
    case LEFT:
      newHead.x--;
      break;
    case RIGHT:
      newHead.x++;
      break;
  }
  
  // Check wall collision
  if (newHead.x < 0 || newHead.x >= COLS || 
      newHead.y < 0 || newHead.y >= ROWS) {
    gameOver = true;
    Serial.println("Game Over - Hit Wall!");
    return;
  }
  
  // Check self collision
  for (int i = 0; i < snakeLength; i++) {
    if (snake[i].x == newHead.x && snake[i].y == newHead.y) {
      gameOver = true;
      Serial.println("Game Over - Hit Self!");
      return;
    }
  }
  
  // Check food collision
  bool ateFood = (newHead.x == food.x && newHead.y == food.y);
  
  if (ateFood) {
    // Grow snake
    snakeLength++;
    score++;
    
    // Increase speed
    moveDelay = max(MIN_SPEED, (int)(moveDelay - SPEED_INCREMENT));
    
    generateFood();
    
    Serial.print("Score: ");
    Serial.print(score);
    Serial.print(" | Speed: ");
    Serial.println(moveDelay);
    
    // Check win condition (snake fills entire matrix)
    if (snakeLength >= NUM_LEDS) {
      gameOver = true;
      Serial.println("You Win! Perfect Game!");
      return;
    }
  } else {
    // Move tail forward (remove last segment)
    for (int i = snakeLength - 1; i > 0; i--) {
      snake[i] = snake[i - 1];
    }
  }
  
  // Add new head
  snake[0] = newHead;
}

void updateDisplay() {
  FastLED.clear();
  
  // Draw snake body
  for (int i = 1; i < snakeLength; i++) {
    int ledIndex = XY(snake[i].x, snake[i].y);
    if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
      leds[ledIndex] = COLOR_SNAKE;
    }
  }
  
  // Draw snake head (brighter)
  int headIndex = XY(snake[0].x, snake[0].y);
  if (headIndex >= 0 && headIndex < NUM_LEDS) {
    leds[headIndex] = COLOR_SNAKE_HEAD;
  }
  
  // Draw food
  int foodIndex = XY(food.x, food.y);
  if (foodIndex >= 0 && foodIndex < NUM_LEDS) {
    leds[foodIndex] = COLOR_FOOD;
  }
}

void showStartScreen() {
  // Animated start screen
  static unsigned long lastAnimation = 0;
  static bool animationState = false;
  
  if (millis() - lastAnimation > 500) {
    FastLED.clear();
    
    if (animationState) {
      // Draw "SNAKE" pattern or simple animation
      for (int i = 0; i < NUM_LEDS; i += 20) {
        leds[i] = CRGB::Green;
      }
    }
    
    animationState = !animationState;
    lastAnimation = millis();
    FastLED.show();
  }
}

void handleStartScreen() {
  // Any button starts the game
  if (digitalRead(BTN_UP) == LOW || digitalRead(BTN_DOWN) == LOW ||
      digitalRead(BTN_LEFT) == LOW || digitalRead(BTN_RIGHT) == LOW) {
    gameStarted = true;
    Serial.println("Game Started!");
    delay(200); // Prevent immediate input
  }
}

void handleGameOver() {
  static unsigned long gameOverStart = 0;
  static bool gameOverInitialized = false;
  
  if (!gameOverInitialized) {
    gameOverStart = millis();
    gameOverInitialized = true;
    
    // Game over animation
    gameOverAnimation();
  }
  
  // Show game over screen for 3 seconds, then restart
  if (millis() - gameOverStart > 3000) {
    initializeGame();
    gameOverInitialized = false;
    showStartScreen();
  }
}

void gameOverAnimation() {
  // Flash red a few times
  for (int flash = 0; flash < 3; flash++) {
    fill_solid(leds, NUM_LEDS, CRGB::Red);
    FastLED.show();
    delay(200);
    
    FastLED.clear();
    FastLED.show();
    delay(200);
  }
  
  // Show final score as brief light pattern
  int scoreDisplay = min(score, NUM_LEDS);
  for (int i = 0; i < scoreDisplay; i++) {
    leds[i] = CRGB::Blue;
  }
  FastLED.show();
}