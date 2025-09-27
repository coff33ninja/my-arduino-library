#include <FastLED.h>

// Matrix Configuration
#define LED_PIN 6
#define COLS 10
#define ROWS 22
#define NUM_LEDS (COLS * ROWS)
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS 100

// Control Configuration
#define BTN_LEFT 2
#define BTN_RIGHT 3
#define POTENTIOMETER_PIN A0
#define USE_POTENTIOMETER true  // Set false to use buttons instead

// Game Configuration
#define PADDLE_WIDTH 3
#define PADDLE_ROW (ROWS - 2)
#define BRICK_ROWS 6
#define BRICK_COLS COLS
#define BALL_SPEED 120  // milliseconds between ball moves
#define PADDLE_SPEED 80 // milliseconds between paddle moves

// Power-up Configuration
#define POWERUP_CHANCE 20  // Percentage chance per brick
#define POWERUP_DURATION 10000  // Duration in milliseconds

CRGB leds[NUM_LEDS];

// Game Objects
struct Ball {
  float x, y;
  float dx, dy;
  bool active;
};

struct Brick {
  bool active;
  int hits;
  CRGB color;
  int points;
};

struct PowerUp {
  int x, y;
  float dy;
  int type;
  bool active;
  unsigned long spawnTime;
};

// Game State
Ball ball;
int paddlePos = COLS / 2;
Brick bricks[BRICK_ROWS][BRICK_COLS];
PowerUp powerUps[5];  // Maximum 5 power-ups at once
int activePowerUps = 0;

int score = 0;
int lives = 3;
int level = 1;
bool gameOver = false;
bool gameStarted = false;
bool levelComplete = false;

// Power-up states
bool widePaddle = false;
bool fastBall = false;
bool multiball = false;
Ball extraBalls[3];
int extraBallCount = 0;

unsigned long powerUpEndTime = 0;
unsigned long lastBallMove = 0;
unsigned long lastPaddleMove = 0;
unsigned long lastPowerUpSpawn = 0;

// Colors
#define COLOR_PADDLE CRGB::White
#define COLOR_BALL CRGB::Yellow
#define COLOR_BACKGROUND CRGB::Black
#define COLOR_WALL CRGB::Gray

// Brick colors by row (top to bottom)
CRGB brickColors[BRICK_ROWS] = {
  CRGB::Red,     // Top row - 50 points
  CRGB::Orange,  // 40 points  
  CRGB::Yellow,  // 30 points
  CRGB::Green,   // 20 points
  CRGB::Blue,    // 15 points
  CRGB::Purple   // Bottom row - 10 points
};

int brickPoints[BRICK_ROWS] = {50, 40, 30, 20, 15, 10};

// Power-up types
enum PowerUpType {
  WIDE_PADDLE = 0,
  FAST_BALL = 1,
  MULTIBALL = 2,
  EXTRA_LIFE = 3
};

CRGB powerUpColors[4] = {
  CRGB::Cyan,    // Wide paddle
  CRGB::Magenta, // Fast ball
  CRGB::White,   // Multiball
  CRGB::Lime     // Extra life
};

void setup() {
  Serial.begin(9600);
  
  // Initialize LED strip
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  
  // Initialize button pins
  if (!USE_POTENTIOMETER) {
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
  }
  
  // Initialize random seed
  randomSeed(analogRead(A1));
  
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
  
  if (levelComplete) {
    handleLevelComplete();
    return;
  }
  
  handleInput();
  updateBall();
  updatePowerUps();
  checkCollisions();
  checkWinLoseConditions();
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
  score = 0;
  lives = 3;
  level = 1;
  gameOver = false;
  gameStarted = false;
  levelComplete = false;
  
  resetLevel();
  
  Serial.println("Breakout Game Initialized");
  Serial.print("Level: ");
  Serial.println(level);
}

void resetLevel() {
  // Reset ball
  ball.x = COLS / 2.0;
  ball.y = ROWS - 4;
  ball.dx = 0.8;
  ball.dy = -0.8;
  ball.active = true;
  
  // Reset paddle
  paddlePos = COLS / 2;
  
  // Reset power-ups
  clearPowerUps();
  
  // Initialize bricks
  for (int row = 0; row < BRICK_ROWS; row++) {
    for (int col = 0; col < BRICK_COLS; col++) {
      bricks[row][col].active = true;
      bricks[row][col].hits = 1;
      bricks[row][col].color = brickColors[row];
      bricks[row][col].points = brickPoints[row];
      
      // Higher level bricks require more hits
      if (level > 3) {
        bricks[row][col].hits = 2;
        bricks[row][col].color = bricks[row][col].color % 128; // Darker
      }
    }
  }
  
  levelComplete = false;
}

void handleInput() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastPaddleMove < PADDLE_SPEED) return;
  
  if (USE_POTENTIOMETER) {
    // Potentiometer control (0-1023 -> 0 to COLS-PADDLE_WIDTH)
    int potValue = analogRead(POTENTIOMETER_PIN);
    int maxPaddleWidth = widePaddle ? 5 : PADDLE_WIDTH;
    paddlePos = map(potValue, 0, 1023, 0, COLS - maxPaddleWidth);
  } else {
    // Button control
    if (digitalRead(BTN_LEFT) == LOW && paddlePos > 0) {
      paddlePos--;
      lastPaddleMove = currentTime;
    }
    if (digitalRead(BTN_RIGHT) == LOW) {
      int maxPaddleWidth = widePaddle ? 5 : PADDLE_WIDTH;
      if (paddlePos < COLS - maxPaddleWidth) {
        paddlePos++;
        lastPaddleMove = currentTime;
      }
    }
  }
}

void updateBall() {
  unsigned long currentTime = millis();
  unsigned long ballSpeed = fastBall ? BALL_SPEED / 2 : BALL_SPEED;
  
  if (currentTime - lastBallMove < ballSpeed) return;
  
  // Update main ball
  updateSingleBall(&ball);
  
  // Update extra balls (multiball power-up)
  for (int i = 0; i < extraBallCount; i++) {
    if (extraBalls[i].active) {
      updateSingleBall(&extraBalls[i]);
    }
  }
  
  lastBallMove = currentTime;
}

void updateSingleBall(Ball* b) {
  if (!b->active) return;
  
  // Update position
  b->x += b->dx;
  b->y += b->dy;
  
  // Wall collisions
  if (b->x <= 0 || b->x >= COLS - 1) {
    b->dx = -b->dx;
    b->x = constrain(b->x, 0, COLS - 1);
  }
  
  if (b->y <= 0) {
    b->dy = -b->dy;
    b->y = 1;
  }
  
  // Ball lost (bottom)
  if (b->y >= ROWS) {
    if (b == &ball) {  // Main ball
      if (extraBallCount > 0) {
        // Promote an extra ball to main ball
        ball = extraBalls[0];
        // Shift remaining extra balls
        for (int i = 0; i < extraBallCount - 1; i++) {
          extraBalls[i] = extraBalls[i + 1];
        }
        extraBallCount--;
      } else {
        lives--;
        if (lives > 0) {
          // Reset ball position
          ball.x = COLS / 2.0;
          ball.y = ROWS - 4;
          ball.dx = 0.8;
          ball.dy = -0.8;
        }
      }
    } else {
      // Extra ball lost
      b->active = false;
    }
  }
}

void checkCollisions() {
  checkBallCollisions(&ball);
  
  for (int i = 0; i < extraBallCount; i++) {
    if (extraBalls[i].active) {
      checkBallCollisions(&extraBalls[i]);
    }
  }
}

void checkBallCollisions(Ball* b) {
  if (!b->active) return;
  
  int ballX = (int)b->x;
  int ballY = (int)b->y;
  
  // Paddle collision
  int paddleWidth = widePaddle ? 5 : PADDLE_WIDTH;
  if (ballY >= PADDLE_ROW && ballY <= PADDLE_ROW + 1) {
    if (ballX >= paddlePos && ballX < paddlePos + paddleWidth) {
      b->dy = -abs(b->dy);
      
      // Add english based on paddle hit position
      float hitPos = (float)(ballX - paddlePos) / paddleWidth;
      b->dx = (hitPos - 0.5) * 1.6;  // -0.8 to 0.8 range
      
      // Ensure minimum vertical speed
      if (abs(b->dy) < 0.4) b->dy = b->dy > 0 ? 0.4 : -0.4;
    }
  }
  
  // Brick collisions
  if (ballY >= 1 && ballY <= BRICK_ROWS) {
    int brickRow = ballY - 1;
    if (brickRow >= 0 && brickRow < BRICK_ROWS && 
        ballX >= 0 && ballX < BRICK_COLS) {
      
      if (bricks[brickRow][ballX].active) {
        // Hit brick
        bricks[brickRow][ballX].hits--;
        
        if (bricks[brickRow][ballX].hits <= 0) {
          // Destroy brick
          score += bricks[brickRow][ballX].points;
          bricks[brickRow][ballX].active = false;
          
          // Chance to spawn power-up
          if (random(100) < POWERUP_CHANCE) {
            spawnPowerUp(ballX, brickRow);
          }
          
          Serial.print("Score: ");
          Serial.println(score);
        } else {
          // Brick damaged but not destroyed (darker color)
          bricks[brickRow][ballX].color = bricks[brickRow][ballX].color % 64;
        }
        
        // Bounce ball
        b->dy = -b->dy;
      }
    }
  }
}

void spawnPowerUp(int x, int y) {
  if (activePowerUps >= 5) return;  // Maximum power-ups
  
  for (int i = 0; i < 5; i++) {
    if (!powerUps[i].active) {
      powerUps[i].x = x;
      powerUps[i].y = y;
      powerUps[i].dy = 0.3;
      powerUps[i].type = random(4);
      powerUps[i].active = true;
      powerUps[i].spawnTime = millis();
      activePowerUps++;
      break;
    }
  }
}

void updatePowerUps() {
  unsigned long currentTime = millis();
  
  // Update falling power-ups
  for (int i = 0; i < 5; i++) {
    if (!powerUps[i].active) continue;
    
    powerUps[i].y += powerUps[i].dy;
    
    // Check paddle collision
    int paddleWidth = widePaddle ? 5 : PADDLE_WIDTH;
    if ((int)powerUps[i].y >= PADDLE_ROW && 
        powerUps[i].x >= paddlePos && 
        powerUps[i].x < paddlePos + paddleWidth) {
      
      activatePowerUp(powerUps[i].type);
      powerUps[i].active = false;
      activePowerUps--;
    }
    
    // Remove if off screen
    if (powerUps[i].y >= ROWS) {
      powerUps[i].active = false;
      activePowerUps--;
    }
  }
  
  // Check power-up expiration
  if (currentTime > powerUpEndTime) {
    clearPowerUps();
  }
}

void activatePowerUp(int type) {
  powerUpEndTime = millis() + POWERUP_DURATION;
  
  switch (type) {
    case WIDE_PADDLE:
      widePaddle = true;
      Serial.println("Power-up: Wide Paddle!");
      break;
      
    case FAST_BALL:
      fastBall = true;
      Serial.println("Power-up: Fast Ball!");
      break;
      
    case MULTIBALL:
      if (extraBallCount < 3) {
        extraBalls[extraBallCount] = ball;
        extraBalls[extraBallCount].dx = -ball.dx;
        extraBallCount++;
        Serial.println("Power-up: Multiball!");
      }
      break;
      
    case EXTRA_LIFE:
      lives++;
      Serial.print("Power-up: Extra Life! Lives: ");
      Serial.println(lives);
      break;
  }
}

void clearPowerUps() {
  widePaddle = false;
  fastBall = false;
  // Don't clear multiball - extra balls persist
}

void checkWinLoseConditions() {
  // Check game over
  if (lives <= 0) {
    gameOver = true;
    Serial.print("Game Over! Final Score: ");
    Serial.println(score);
    return;
  }
  
  // Check level complete
  bool anyBricksLeft = false;
  for (int row = 0; row < BRICK_ROWS; row++) {
    for (int col = 0; col < BRICK_COLS; col++) {
      if (bricks[row][col].active) {
        anyBricksLeft = true;
        break;
      }
    }
    if (anyBricksLeft) break;
  }
  
  if (!anyBricksLeft) {
    levelComplete = true;
    level++;
    Serial.print("Level Complete! Starting Level: ");
    Serial.println(level);
  }
}

void updateDisplay() {
  FastLED.clear();
  
  // Draw bricks
  for (int row = 0; row < BRICK_ROWS; row++) {
    for (int col = 0; col < BRICK_COLS; col++) {
      if (bricks[row][col].active) {
        int ledIndex = XY(col, row + 1);
        if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
          leds[ledIndex] = bricks[row][col].color;
        }
      }
    }
  }
  
  // Draw paddle
  int paddleWidth = widePaddle ? 5 : PADDLE_WIDTH;
  for (int i = 0; i < paddleWidth; i++) {
    int ledIndex = XY(paddlePos + i, PADDLE_ROW);
    if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
      leds[ledIndex] = COLOR_PADDLE;
    }
  }
  
  // Draw main ball
  if (ball.active) {
    int ballIndex = XY((int)ball.x, (int)ball.y);
    if (ballIndex >= 0 && ballIndex < NUM_LEDS) {
      leds[ballIndex] = COLOR_BALL;
    }
  }
  
  // Draw extra balls
  for (int i = 0; i < extraBallCount; i++) {
    if (extraBalls[i].active) {
      int ballIndex = XY((int)extraBalls[i].x, (int)extraBalls[i].y);
      if (ballIndex >= 0 && ballIndex < NUM_LEDS) {
        leds[ballIndex] = CRGB::Orange;  // Different color for extra balls
      }
    }
  }
  
  // Draw power-ups
  for (int i = 0; i < 5; i++) {
    if (powerUps[i].active) {
      int powerUpIndex = XY(powerUps[i].x, (int)powerUps[i].y);
      if (powerUpIndex >= 0 && powerUpIndex < NUM_LEDS) {
        leds[powerUpIndex] = powerUpColors[powerUps[i].type];
      }
    }
  }
  
  // Draw lives indicator (bottom row)
  for (int i = 0; i < min(lives, COLS); i++) {
    int lifeIndex = XY(i, ROWS - 1);
    if (lifeIndex >= 0 && lifeIndex < NUM_LEDS) {
      leds[lifeIndex] = CRGB::Red;
    }
  }
}

void showStartScreen() {
  static unsigned long lastAnimation = 0;
  static int animationStep = 0;
  
  if (millis() - lastAnimation > 200) {
    FastLED.clear();
    
    // Animated rainbow sweep
    for (int i = 0; i < COLS; i++) {
      int hue = (animationStep * 10 + i * 25) % 255;
      for (int j = 0; j < ROWS; j += 3) {
        int ledIndex = XY(i, j);
        if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
          leds[ledIndex] = CHSV(hue, 255, 128);
        }
      }
    }
    
    animationStep++;
    lastAnimation = millis();
    FastLED.show();
  }
}

void handleStartScreen() {
  // Any button or potentiometer movement starts the game
  if (USE_POTENTIOMETER) {
    static int lastPotValue = -1;
    int currentPotValue = analogRead(POTENTIOMETER_PIN);
    if (lastPotValue >= 0 && abs(currentPotValue - lastPotValue) > 50) {
      gameStarted = true;
      Serial.println("Game Started!");
    }
    lastPotValue = currentPotValue;
  } else {
    if (digitalRead(BTN_LEFT) == LOW || digitalRead(BTN_RIGHT) == LOW) {
      gameStarted = true;
      Serial.println("Game Started!");
      delay(200); // Prevent immediate input
    }
  }
}

void handleGameOver() {
  static unsigned long gameOverStart = 0;
  static bool gameOverInitialized = false;
  
  if (!gameOverInitialized) {
    gameOverStart = millis();
    gameOverInitialized = true;
    gameOverAnimation();
  }
  
  // Show game over screen for 5 seconds, then restart
  if (millis() - gameOverStart > 5000) {
    initializeGame();
    gameOverInitialized = false;
    showStartScreen();
  }
}

void handleLevelComplete() {
  static unsigned long levelCompleteStart = 0;
  static bool levelCompleteInitialized = false;
  
  if (!levelCompleteInitialized) {
    levelCompleteStart = millis();
    levelCompleteInitialized = true;
    levelCompleteAnimation();
  }
  
  // Show celebration for 2 seconds, then next level
  if (millis() - levelCompleteStart > 2000) {
    resetLevel();
    levelCompleteInitialized = false;
  }
}

void gameOverAnimation() {
  // Cascade fall effect
  for (int row = 0; row < ROWS; row++) {
    FastLED.clear();
    
    for (int col = 0; col < COLS; col++) {
      for (int r = row; r < ROWS; r++) {
        int ledIndex = XY(col, r);
        if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
          leds[ledIndex] = CRGB::Red;
        }
      }
    }
    
    FastLED.show();
    delay(100);
  }
  
  delay(500);
  
  // Show final score (simplified display)
  FastLED.clear();
  int scoreDisplay = min(score / 10, NUM_LEDS);
  for (int i = 0; i < scoreDisplay; i++) {
    leds[i] = CRGB::Blue;
  }
  FastLED.show();
}

void levelCompleteAnimation() {
  // Rainbow explosion effect
  for (int radius = 0; radius < max(COLS, ROWS); radius++) {
    FastLED.clear();
    
    int centerX = COLS / 2;
    int centerY = ROWS / 2;
    
    for (int x = 0; x < COLS; x++) {
      for (int y = 0; y < ROWS; y++) {
        int distance = abs(x - centerX) + abs(y - centerY);
        if (distance == radius) {
          int ledIndex = XY(x, y);
          if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
            leds[ledIndex] = CHSV((radius * 30) % 255, 255, 255);
          }
        }
      }
    }
    
    FastLED.show();
    delay(150);
  }
}