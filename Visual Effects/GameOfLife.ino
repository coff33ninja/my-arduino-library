#include <FastLED.h>

// Matrix dimensions
#define MATRIX_WIDTH 22
#define MATRIX_HEIGHT 10
#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT)
#define LED_PIN 6

// Simulation parameters
#define SIMULATION_SPEED 100 // ms delay between generations

// Uncomment the line below to enable speed control via a potentiometer on pin A1
// #define USE_POT_SPEED_CONTROL

CRGB leds[NUM_LEDS];

// Cell state arrays
byte world[MATRIX_WIDTH][MATRIX_HEIGHT];
byte next_world[MATRIX_WIDTH][MATRIX_HEIGHT];
byte cellAge[MATRIX_WIDTH][MATRIX_HEIGHT];

// Function to get LED index from (x, y) in serpentine matrix
int getLEDIndex(int x, int y) {
  if (y % 2 == 0) {
    return y * MATRIX_WIDTH + x;
  } else {
    return y * MATRIX_WIDTH + (MATRIX_WIDTH - 1 - x);
  }
}

void setup() {
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(32);
  Serial.begin(115200);

  // Initialize world with a random pattern
  randomSeed(analogRead(A0));
  for (int x = 0; x < MATRIX_WIDTH; x++) {
    for (int y = 0; y < MATRIX_HEIGHT; y++) {
      world[x][y] = random(0, 2); // 0 = dead, 1 = alive
      cellAge[x][y] = 0;
    }
  }
}

void drawWorld() {
  for (int x = 0; x < MATRIX_WIDTH; x++) {
    for (int y = 0; y < MATRIX_HEIGHT; y++) {
      if (world[x][y] == 1) {
        // Color based on age
        if (cellAge[x][y] < 5) {
          leds[getLEDIndex(x, y)] = CRGB::Red;
        } else if (cellAge[x][y] < 10) {
          leds[getLEDIndex(x, y)] = CRGB::Orange;
        } else if (cellAge[x][y] < 15) {
          leds[getLEDIndex(x, y)] = CRGB::Yellow;
        } else {
          leds[getLEDIndex(x, y)] = CRGB::Green;
        }
      } else {
        leds[getLEDIndex(x, y)] = CRGB::Black;
      }
    }
  }
  FastLED.show();
}

int countNeighbors(int x, int y) {
  int count = 0;
  for (int i = -1; i <= 1; i++) {
    for (int j = -1; j <= 1; j++) {
      if (i == 0 && j == 0) continue;
      int neighbor_x = (x + i + MATRIX_WIDTH) % MATRIX_WIDTH;
      int neighbor_y = (y + j + MATRIX_HEIGHT) % MATRIX_HEIGHT;
      count += world[neighbor_x][neighbor_y];
    }
  }
  return count;
}

void computeNextGeneration() {
  for (int x = 0; x < MATRIX_WIDTH; x++) {
    for (int y = 0; y < MATRIX_HEIGHT; y++) {
      int neighbors = countNeighbors(x, y);
      // Rule 1: Any live cell with fewer than two live neighbours dies (underpopulation).
      if ((world[x][y] == 1) && (neighbors < 2)) {
        next_world[x][y] = 0;
        cellAge[x][y] = 0;
      }
      // Rule 2: Any live cell with two or three live neighbours lives on to the next generation.
      else if ((world[x][y] == 1) && (neighbors == 2 || neighbors == 3)) {
        next_world[x][y] = 1;
        cellAge[x][y]++;
      }
      // Rule 3: Any live cell with more than three live neighbours dies (overpopulation).
      else if ((world[x][y] == 1) && (neighbors > 3)) {
        next_world[x][y] = 0;
        cellAge[x][y] = 0;
      }
      // Rule 4: Any dead cell with exactly three live neighbours becomes a live cell (reproduction).
      else if ((world[x][y] == 0) && (neighbors == 3)) {
        next_world[x][y] = 1;
        cellAge[x][y] = 1;
      }
      else {
        next_world[x][y] = world[x][y];
      }
    }
  }

  // Copy next world to current world
  memcpy(world, next_world, sizeof(world));
}

void loop() {
  drawWorld();
  computeNextGeneration();
  #ifdef USE_POT_SPEED_CONTROL
    int potValue = analogRead(A1);
    int simulationSpeed = map(potValue, 0, 1023, 10, 500);
    delay(simulationSpeed);
  #else
    delay(SIMULATION_SPEED);
  #endif
}
