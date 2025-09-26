#include <Arduino.h>
#include <FastLED.h>
#include <avr/pgmspace.h>

#define LED_PIN     6
#define NUM_LEDS    220
#define BRIGHTNESS  32
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

#define ROWS 10
#define COLS 22

// Dino dimensions and position
#define DINO_WIDTH 3
#define DINO_HEIGHT 5
#define DINO_X 3
#define GROUND_Y 9

// Physics constants
#define MAX_JUMP_HEIGHT 3
#define JUMP_VELOCITY 2
#define GRAVITY 1

// Colors with more variety
CRGB DINO_COLOR = CRGB::Lime;
CRGB CACTUS_COLOR = CRGB::ForestGreen;
CRGB GROUND_COLOR = CRGB(139, 69, 19); // SaddleBrown
CRGB SKY_COLOR = CRGB(25, 25, 112);    // MidnightBlue
CRGB STAR_COLOR = CRGB::White;

// Enhanced Dino frames - 4 running frames + jump frame + duck frame (fixed orientation: facing right, head at top)
const bool dinoFrames[6][DINO_HEIGHT][DINO_WIDTH] PROGMEM = {
  // Frame 0: Running - leg back, arm forward
  {
    {1,1,0},  // Head (top row, facing right)
    {1,1,1},  // Body
    {1,1,0},  // Body
    {1,0,1},  // Legs spread
    {1,0,0}   // Leg back (bottom row)
  },
  // Frame 1: Running - leg forward, arm back  
  {
    {1,1,0},  // Head (top row, facing right)
    {1,1,1},  // Body
    {1,1,0},  // Body
    {1,1,0},  // Legs together
    {0,1,0}   // Leg forward (bottom row)
  },
  // Frame 2: Running - both legs mid-stride
  {
    {1,1,0},  // Head (top row, facing right)
    {1,1,1},  // Body
    {1,1,0},  // Body
    {0,0,0},  // Transition
    {1,0,1}   // Both legs visible (bottom row)
  },
  // Frame 3: Running - leg back, arm forward (variation)
  {
    {1,1,0},  // Head (top row, facing right)
    {1,1,1},  // Body
    {1,1,0},  // Body
    {1,0,1},  // Legs spread
    {0,1,1}   // Leg back variation (bottom row)
  },
  // Frame 4: Jumping
  {
    {1,1,0},  // Head (top row, facing right)
    {1,1,1},  // Body
    {1,1,0},  // Body
    {1,1,1},  // Legs tucked
    {0,0,0}   // No ground contact (bottom row)
  },
  // Frame 5: Ducking (lower profile)
  {
    {0,0,0},  // No head visible (ducked down)
    {1,1,0},  // Head lowered
    {1,1,1},  // Body compressed
    {1,1,0},  // Body
    {1,0,1}   // Legs spread for balance
  }
};

// Obstacle system
struct Obstacle {
  int x;
  int height;
  int width;
  bool active;
};

#define MAX_OBSTACLES 8
Obstacle obstacles[MAX_OBSTACLES];
int activeObstacles = 0;
unsigned long lastSpawn = 0;
unsigned long frameTime = 0;

// Game state
struct GameState {
  int dinoY;
  int jumpVelocity;
  bool isJumping;
  bool isDucking;
  int currentFrame;
  int speed;
  unsigned long score;
  unsigned long lastFrameSwitch;
  bool gameOver;
  int lives;
  bool invincible;  // New: brief invincibility after hit
  unsigned long invincibilityTimer;
} game;

// Enhanced Star field for background - more random placement
struct Star {
  int x, y;
  CRGB color;
  uint8_t brightness;  // Individual brightness level
  unsigned long lastTwinkle;
  bool visible;
};
#define NUM_STARS 6  // Reduced number for more space
Star stars[NUM_STARS];

// Button input with debouncing
#define BUTTON_PIN 2
#define DUCK_PIN 3
bool buttonPressed = false;
bool lastButtonState = false;
bool lastDuckState = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// Coordinate mapping for serpentine LED arrangement - matches your text display
int XY(int x, int y) {
  if (x < 0 || x >= COLS || y < 0 || y >= ROWS) return -1;
  
  int realX = (COLS - 1) - x;
  if (y % 2 == 0) {
    return y * COLS + realX;
  } else {
    return y * COLS + (COLS - 1 - realX);
  }
}

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 1000); // Reduced for power management
  
  // Initialize button
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  randomSeed(analogRead(0));
  
  // Power-up sequence
  FastLED.clear();
  FastLED.show();
  delay(500); // Let power stabilize
  
  initializeGame();
  initializeStars();
  
  Serial.println("Enhanced Dino Game Started!");
}

void loop() {
  unsigned long now = millis();
  
  // Handle input with debouncing
  handleInput();
  
  // Game timing - variable frame rate based on speed
  int targetFrameTime = max(50, 150 - (game.speed * 10));
  if (now - frameTime >= targetFrameTime) {
    if (!game.gameOver) {
      updateGame();
      game.score += game.speed;
    }
    drawScene();
    
    // Adjust brightness based on game state
    if (game.gameOver) {
      FastLED.setBrightness(BRIGHTNESS / 4);
    } else {
      FastLED.setBrightness(BRIGHTNESS);
    }
    
    FastLED.show();
    frameTime = now;
  }
  
  // Enhanced animation timing
  if (now - game.lastFrameSwitch >= getAnimationDelay()) {
    updateAnimation();
    game.lastFrameSwitch = now;
  }
  
  // Spawn obstacles
  if (!game.gameOver && now - lastSpawn > getSpawnDelay()) {
    spawnObstacle();
    lastSpawn = now;
  }
  
  // Update invincibility timer
  if (game.invincible && now - game.invincibilityTimer > 1000) {
    game.invincible = false;
  }
  
  // Check for game over reset
  if (game.gameOver && (digitalRead(BUTTON_PIN) == LOW || now % 3000 < 100)) {
    initializeGame();
  }
}

void handleInput() {
  bool jumpReading = digitalRead(BUTTON_PIN) == LOW;
  bool duckReading = digitalRead(DUCK_PIN) == LOW;
  
  // Jump button debouncing
  if (jumpReading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (jumpReading != buttonPressed) {
      buttonPressed = jumpReading;
      if (buttonPressed && !game.isJumping && !game.gameOver && !game.isDucking) {
        startJump();
      }
    }
  }
  
  // Duck handling (immediate response for better control)
  game.isDucking = duckReading && !game.isJumping && !game.gameOver;
  
  lastButtonState = jumpReading;
  lastDuckState = duckReading;
}

void initializeGame() {
  game.dinoY = 0;
  game.jumpVelocity = 0;
  game.isJumping = false;
  game.isDucking = false;
  game.currentFrame = 0;
  game.speed = 1;
  game.score = 0;
  game.lastFrameSwitch = 0;
  game.gameOver = false;
  game.lives = 3;
  game.invincible = false;
  
  // Clear obstacles
  activeObstacles = 0;
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    obstacles[i].active = false;
  }
  
  Serial.println("Game Started! Score: 0 | Lives: 3");
}

void initializeStars() {
  // Clear all star positions first
  for (int i = 0; i < NUM_STARS; i++) {
    stars[i].x = -1;
    stars[i].y = -1;
    stars[i].visible = false;
  }
  
  // Place stars more randomly, ensuring no overlap with UI areas
  for (int i = 0; i < NUM_STARS; i++) {
    bool placed = false;
    int attempts = 0;
    
    while (!placed && attempts < 50) {  // Prevent infinite loops
      int x = random(2, COLS - 4);  // Leave space for UI on sides
      int y = random(1, GROUND_Y - 1);  // Above ground, below UI row
      
      // Check if this position overlaps with another star or UI area
      bool validPosition = true;
      
      // Don't place in UI areas
      if (y <= 2 && (x <= 5 || x >= COLS - 4)) {
        validPosition = false;
      }
      
      // Check distance from other stars (minimum 3 spaces apart)
      for (int j = 0; j < i; j++) {
        if (stars[j].x >= 0) {
          int distX = abs(stars[j].x - x);
          int distY = abs(stars[j].y - y);
          if (distX < 3 && distY < 3) {
            validPosition = false;
            break;
          }
        }
      }
      
      if (validPosition) {
        stars[i].x = x;
        stars[i].y = y;
        
        // Random star colors - cooler tones for night sky
        uint8_t r = random(100, 200);
        uint8_t g = random(50, 150);
        uint8_t b = random(150, 255);
        stars[i].color = CRGB(r, g, b);
        
        // Random base brightness (very dim to start)
        stars[i].brightness = random(10, 40);  // Much dimmer base
        stars[i].lastTwinkle = random(0, 5000);  // Random start times
        stars[i].visible = true;
        placed = true;
      }
      
      attempts++;
    }
    
    if (!placed) {
      // If couldn't find a good spot, just place it anyway
      stars[i].x = random(3, COLS - 3);
      stars[i].y = random(2, GROUND_Y - 2);
      stars[i].color = CRGB(random(120, 180), random(80, 120), random(180, 255));
      stars[i].brightness = random(8, 25);
      stars[i].lastTwinkle = random(0, 5000);
      stars[i].visible = true;
    }
    
    Serial.print("Star ");
    Serial.print(i);
    Serial.print(" at (");
    Serial.print(stars[i].x);
    Serial.print(", ");
    Serial.print(stars[i].y);
    Serial.print(") brightness: ");
    Serial.println(stars[i].brightness);
  }
}

void updateGame() {
  // Update physics
  updatePhysics();
  
  // Move obstacles
  moveObstacles();
  
  // Enhanced collision detection
  if (checkCollision()) {
    handleCollision();
  }
  
  // Increase difficulty
  if (game.score > 0 && game.score % 1000 == 0 && game.speed < 5) {
    game.speed = min(game.speed + 1, 5);
    Serial.println("Speed increased to: " + String(game.speed));
  }
  
  // Auto-jump AI (smart obstacle avoidance) - DISABLED for manual play
  // Uncomment the next 3 lines if you want AI assistance
  // if (!game.isJumping && !game.invincible && shouldJump()) {
  //   startJump();
  // }
}

void updateAnimation() {
  if (game.isJumping) {
    game.currentFrame = 4; // Jump frame
  } else if (game.isDucking) {
    game.currentFrame = 5; // Duck frame
  } else {
    // Cycle through 4 running frames for smoother animation
    if (game.speed == 1) {
      game.currentFrame = (game.currentFrame + 1) % 4; // Slower at low speed
    } else {
      game.currentFrame = (game.currentFrame + 1) % 4; // Faster cycling
    }
  }
}

int getAnimationDelay() {
  // Animation speed scales with game speed
  return max(80, 200 - (game.speed * 30));
}

void updatePhysics() {
  if (game.isJumping) {
    game.dinoY += game.jumpVelocity;
    game.jumpVelocity -= GRAVITY;
    
    // Land
    if (game.dinoY <= 0) {
      game.dinoY = 0;
      game.jumpVelocity = 0;
      game.isJumping = false;
    }
    
    // Clamp max height
    if (game.dinoY > MAX_JUMP_HEIGHT) {
      game.dinoY = MAX_JUMP_HEIGHT;
      game.jumpVelocity = 0;
    }
  }
}

void moveObstacles() {
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (obstacles[i].active) {
      obstacles[i].x -= game.speed; // Move left toward dino
      
      // Remove off-screen obstacles (left side)
      if (obstacles[i].x < -obstacles[i].width) {
        obstacles[i].active = false;
        activeObstacles--;
      }
    }
  }
}

bool checkCollision() {
  if (game.invincible) return false;
  
  // Get dino bounding box - adjust for ducking
  int dinoLeft = DINO_X;
  int dinoRight = DINO_X + DINO_WIDTH - 1;
  int dinoBottom = GROUND_Y - game.dinoY;
  
  // Ducking makes dino shorter (reduce height by 1)
  int dinoHeight = game.isDucking ? DINO_HEIGHT - 1 : DINO_HEIGHT;
  int dinoTop = dinoBottom - dinoHeight + 1;
  
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (!obstacles[i].active) continue;
    
    int obsLeft = obstacles[i].x;
    int obsRight = obstacles[i].x + obstacles[i].width - 1;
    int obsTop = GROUND_Y - obstacles[i].height + 1;
    int obsBottom = GROUND_Y;
    
    // Enhanced overlap detection with jump clearance and ducking
    if (!(dinoRight < obsLeft || dinoLeft > obsRight ||
          dinoBottom < obsTop || dinoTop > obsBottom)) {
      
      // Check if dino is jumping over (head above obstacle)
      if (game.isJumping && dinoTop <= obsBottom) {
        // Dino is jumping but not high enough
        return true;
      } else if (!game.isJumping && !game.isDucking && dinoBottom >= obsTop) {
        // Dino is on ground and obstacle is at ground level
        return true;
      } else if (!game.isJumping && game.isDucking && dinoBottom >= obsTop) {
        // Check if ducking helps avoid collision (only for tall obstacles)
        if (obstacles[i].height > 2) {
          // Ducking might help with tall obstacles
          continue;
        } else {
          return true;
        }
      }
    }
  }
  return false;
}

void handleCollision() {
  game.lives--;
  Serial.println("Hit! Lives remaining: " + String(game.lives));
  
  // Brief invincibility
  game.invincible = true;
  game.invincibilityTimer = millis();
  
  if (game.lives <= 0) {
    game.gameOver = true;
    Serial.println("Game Over! Final Score: " + String(game.score));
  } else {
    // Remove nearby obstacle for brief relief
    for (int i = 0; i < MAX_OBSTACLES; i++) {
      if (obstacles[i].active && obstacles[i].x >= DINO_X - 2 && obstacles[i].x <= DINO_X + DINO_WIDTH + 2) {
        obstacles[i].active = false;
        activeObstacles--;
        break;
      }
    }
  }
}

bool shouldJump() {
  // Enhanced AI - look ahead for incoming obstacles
  // OPTIONAL: Comment out this entire function if you want manual control only
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (!obstacles[i].active) continue;
    
    int distance = obstacles[i].x - (DINO_X + DINO_WIDTH);
    int jumpDistance = game.speed * 8; // Approximate jump timing
    
    // Jump for tall obstacles that need clearing
    if (distance > 0 && distance <= jumpDistance && obstacles[i].height >= 2) {
      // Check if we have enough time to jump
      int timeToCollision = distance / game.speed;
      if (timeToCollision >= 3) { // Need at least 3 frames to start jump
        return true;
      }
    }
  }
  return false;
}

void startJump() {
  if (!game.isJumping) {
    game.isJumping = true;
    game.jumpVelocity = JUMP_VELOCITY;
  }
}

unsigned long getSpawnDelay() {
  // Dynamic spawn rate based on speed and score
  unsigned long baseDelay = max(800UL, 2500UL - (game.speed * 300UL));
  return baseDelay + random(0, 1000);
}

void spawnObstacle() {
  if (activeObstacles >= MAX_OBSTACLES) return;
  
  // Find empty slot
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (!obstacles[i].active) {
      obstacles[i].x = COLS - 1; // Start at right edge
      obstacles[i].height = random(2, min(5, game.speed + 2)); // Taller with speed
      obstacles[i].width = random(1, min(3, game.speed + 1)); // Wider with speed
      obstacles[i].active = true;
      activeObstacles++;
      break;
    }
  }
}

void drawScene() {
  // Clear display with dimmed sky background
  CRGB dimSky = SKY_COLOR;
  dimSky.nscale8(25);  // Dim background to 25% brightness to prevent overpowering
  fill_solid(leds, NUM_LEDS, dimSky);
  
  // Draw twinkling stars (much dimmer)
  drawStars();
  
  // Draw ground with enhanced texture
  drawGround();
  
  // Draw obstacles
  drawObstacles();
  
  // Draw dino
  drawDino();
  
  // Draw UI elements
  drawUI();
}

void drawStars() {
  unsigned long now = millis();
  
  for (int i = 0; i < NUM_STARS; i++) {
    if (!stars[i].visible || stars[i].x < 0) continue;
    
    // Individual twinkle timing for each star
    unsigned long twinklePhase = (now - stars[i].lastTwinkle) % 3000; // 3 second cycle
    
    // Calculate current brightness based on twinkle phase
    uint8_t currentBrightness = stars[i].brightness;
    
    if (twinklePhase < 200) {
      // Rising phase - very gentle
      currentBrightness = stars[i].brightness * twinklePhase / 200;
    } else if (twinklePhase < 400) {
      // Peak brightness - still dim
      currentBrightness = stars[i].brightness * 2;
      currentBrightness = min(currentBrightness, 60); // Cap at very dim
    } else if (twinklePhase < 2200) {
      // Falling phase - fade out slowly
      currentBrightness = stars[i].brightness * (2200 - twinklePhase) / 1800;
    } else {
      // Off phase - completely dark
      currentBrightness = 0;
    }
    
    // Only draw if bright enough to see
    if (currentBrightness > 5) {
      int ledIndex = XY(stars[i].x, stars[i].y);
      if (ledIndex >= 0) {
        CRGB starColor = stars[i].color;
        starColor.nscale8(currentBrightness); // Apply individual brightness
        leds[ledIndex] = starColor;
      }
    }
    
    // Update last twinkle time occasionally for variation
    if (random(100) < 2) {
      stars[i].lastTwinkle = now;
    }
  }
}

void drawGround() {
  for (int x = 0; x < COLS; x++) {
    int ledIndex = XY(x, GROUND_Y);
    if (ledIndex >= 0) {
      // Enhanced ground texture with more variation
      CRGB groundColor = GROUND_COLOR;
      int texture = (x + (millis()/150)) % 6;
      
      switch(texture) {
        case 0: case 3:
          groundColor.nscale8(120); // Darker patches
          break;
        case 1: case 4:
          groundColor = CRGB(160, 82, 45); // Slightly lighter
          break;
        case 2: case 5:
          groundColor.nscale8(180); // Brighter highlights
          break;
      }
      leds[ledIndex] = groundColor;
    }
  }
}

void drawObstacles() {
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (!obstacles[i].active) continue;
    
    for (int dx = 0; dx < obstacles[i].width; dx++) {
      for (int dy = 0; dy < obstacles[i].height; dy++) {
        int x = obstacles[i].x + dx;
        int y = GROUND_Y - dy;
        int ledIndex = XY(x, y);
        
        if (ledIndex >= 0 && y < ROWS) {
          CRGB cactusColor = CACTUS_COLOR;
          
          // Gradient effect - brighter at top
          if (dy == obstacles[i].height - 1) {
            cactusColor = CRGB::Green; // Bright green top
          } else if (dy == obstacles[i].height - 2) {
            cactusColor = CRGB(34, 139, 34); // Darker green
          }
          
          // Add some randomness for organic look
          if (random(10) < 2) {
            cactusColor.nscale8(80);
          }
          
          leds[ledIndex] = cactusColor;
        }
      }
    }
  }
}

void drawDino() {
  CRGB dinoColor = DINO_COLOR;
  
  // Invincibility flash effect
  if (game.invincible && (millis() / 100) % 2) {
    dinoColor = CRGB::White;
  } else if (game.gameOver) {
    dinoColor = CRGB::Red;
  }
  
  // Draw from top to bottom: dy=0 is top of dino (head), dy=4 is bottom (feet)
  int dinoTopY = (GROUND_Y - DINO_HEIGHT + 1) - game.dinoY;
  for (int dy = 0; dy < DINO_HEIGHT; dy++) {
    int screenY = dinoTopY + dy;
    if (screenY < 0 || screenY >= ROWS) continue;
    
    for (int dx = 0; dx < DINO_WIDTH; dx++) {
      int screenX = DINO_X + dx;
      if (screenX >= COLS) continue;
      
      bool pixel = pgm_read_byte(&dinoFrames[game.currentFrame][dy][dx]);
      if (pixel) {
        int ledIndex = XY(screenX, screenY);
        if (ledIndex >= 0) {
          leds[ledIndex] = dinoColor;
        }
      }
    }
  }
}

void drawUI() {
  // Enhanced lives indicator (top right) - heart shapes
  for (int i = 0; i < min(game.lives, 3); i++) {
    int heartX = COLS - 1 - (i * 2);
    int heartY = 0;
    
    // Simple heart shape (2x1)
    int heartIndex1 = XY(heartX, heartY);
    int heartIndex2 = XY(heartX - 1, heartY);
    
    if (heartIndex1 >= 0) leds[heartIndex1] = CRGB::Red;
    if (heartIndex2 >= 0) leds[heartIndex2] = CRGB::Red;
  }
  
  // Speed indicator (top left) - bar graph
  for (int i = 0; i < min(game.speed, 5); i++) {
    int ledIndex = XY(i, 0);
    if (ledIndex >= 0) {
      leds[ledIndex] = CRGB::Blue;
      // Make it brighter for higher speeds
      if (i >= 3) leds[ledIndex].nscale8(200);
    }
  }
  
  // Score display (rows 1-2)
  drawScore();
}

void drawScore() {
  // Enhanced score visualization - progress bars across top rows
  unsigned long displayScore = min(game.score / 100, 44UL); // Max 4400 points
  int barLength = min(displayScore, 20UL); // One row
  
  // Draw score bar on row 1
  for (int i = 0; i < barLength; i++) {
    int x = i;
    int y = 1;
    int ledIndex = XY(x, y);
    if (ledIndex >= 0) {
      CRGB scoreColor = CRGB::Yellow;
      // Gradient effect - brighter at start
      scoreColor.nscale8(100 + (20 * (barLength - i)));
      leds[ledIndex] = scoreColor;
    }
  }
  
  // Draw score multiplier indicator on row 2 (speed-based)
  int multiplierX = min((int)displayScore, 19);
  int ledIndex = XY(multiplierX, 2);
  if (ledIndex >= 0 && game.speed > 1) {
    leds[ledIndex] = CRGB::Orange;
  }
}

// Test function for matrix mapping (uncomment to test)
// void testMatrix() {
//   Serial.println("Testing matrix mapping...");
//   for (int y = 0; y < ROWS; y++) {
//     fill_solid(leds, NUM_LEDS, CRGB::Black);
//     for (int x = 0; x < COLS; x++) {
//       int idx = XY(x, y);
//       if (idx >= 0) {
//         leds[idx] = CHSV((x+y)*15, 255, 100);
//       }
//     }
//     FastLED.show();
//     delay(200);
//   }
//   Serial.println("Matrix test complete!");
// }
