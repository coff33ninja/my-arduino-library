# Arduino LED Matrix Maze Runner

A procedurally generated maze game for WS2812B LED matrices featuring advanced pathfinding AI, intelligent hint systems, and infinite level progression.

![Maze Runner Demo](https://img.shields.io/badge/Status-Complete-brightgreen) ![Platform](https://img.shields.io/badge/Platform-Arduino-blue)

## Features

### 🎮 **Gameplay Mechanics**
- **Procedural Maze Generation**: Each level features a unique, algorithmically generated maze using recursive backtracking.
- **Guaranteed Solvability**: Every maze has a valid path from start (green) to finish (yellow).
- **Infinite Progression**: Unlimited levels with increasing complexity and visual variety.
- **Move Optimization**: Track moves taken and compare against optimal solution path.
- **Personal Best System**: Records and displays your best performance per level.
- **Four-Directional Movement**: Smooth responsive controls for exploring maze paths.

### 🧠 **Advanced AI Features**
- **A* Pathfinding Algorithm**: Calculates optimal solution path using industry-standard pathfinding.
- **Intelligent Hint System**: Shows next 3 optimal moves when requested without spoiling entire solution.
- **Full Solution Display**: Long-press hint button to see complete optimal path animated step-by-step.
- **Real-Time Pathfinding**: Solution recalculated for each new maze ensuring accuracy.
- **Distance Heuristics**: A* implementation uses Manhattan distance for efficient pathfinding.

### 🌟 **Visual Effects & Feedback**
- **Color-Coded Elements**: Blue walls, black paths, green start, yellow finish for clear navigation.
- **Player Visualization**: Red player marker stands out against maze background.
- **Hint Visualization**: Cyan highlights show next recommended moves without overwhelming display.
- **Solution Animation**: Magenta path traces optimal route with smooth step-by-step revelation.
- **Victory Celebration**: Rainbow spiral explosion from finish point celebrating level completion.
- **Animated Start Screen**: Engaging maze preview with shifting blue pattern animation.

### 🕹️ **Interactive Hint System**
- **Timed Hints**: Short press shows next 3 moves for 3 seconds, then auto-hides.
- **Full Solution Mode**: Long press reveals complete path with animated progression.
- **Smart Context Awareness**: Hints adapt to current player position within solution path.
- **Toggle Functionality**: Easy switching between hint modes during gameplay.

## Hardware Requirements

### Components
- **Arduino Nano** (or compatible microcontroller)
- **8x20 WS2812B LED Matrix** (160 LEDs total)
- **5x Push Buttons** for directional control and hints
- **5x 10kΩ Pull-up Resistors** for button stability
- **Power Supply**: 5V with sufficient amperage for 160 LEDs (recommended: 5V 5A)
- **Capacitor**: 470µF-1000µF across power lines for voltage stabilization
- **Resistor**: 330Ω on LED data line for signal protection

### Wiring Diagram
```
Arduino Nano -> Component
=====================================
Pin 6        -> LED Matrix Data (via 330Ω resistor)
Pin 2        -> Up Button (other pin to GND)
Pin 3        -> Down Button (other pin to GND)  
Pin 4        -> Left Button (other pin to GND)
Pin 5        -> Right Button (other pin to GND)
Pin 7        -> Hint Button (other pin to GND)
5V           -> LED Matrix VCC
GND          -> LED Matrix GND, All Button Commons
A0           -> Floating (used for randomSeed)
```

## Memory Usage

This sketch has been optimized to run efficiently on memory-constrained devices like the Arduino Nano and Uno. It uses approximately 800 bytes of RAM, leaving sufficient memory for stable operation.

- **PROGMEM**: All static data, such as color palettes and maze generation tables, is stored in flash memory to conserve RAM.
- **Optimized Data Structures**: The A* pathfinding algorithm and maze storage use memory-efficient data structures.

## Configuration Options

The following parameters can be easily customized at the top of the `.ino` file.

### Maze Generation Settings
```cpp
#define MAX_PATH_LENGTH 80      // Maximum solution path length
```

### Visual & Timing Settings
```cpp
#define HINT_DURATION 3000       // Hint display time (ms)
#define SOLUTION_SPEED 200       // Solution animation speed (ms/step)
#define BRIGHTNESS 100           // LED brightness (0-255)
```

### Color Customization

Colors are stored in PROGMEM (flash memory) to save RAM. They can be customized in the `col...[]` arrays at the top of the sketch.

```cpp
const uint8_t colWall[]     PROGMEM = {  0,   0, 255};   // blue
const uint8_t colPath[]     PROGMEM = {  0,   0,   0};   // black
const uint8_t colStart[]    PROGMEM = {  0, 255,   0};   // green
const uint8_t colFinish[]   PROGMEM = {255, 255,   0};   // yellow
const uint8_t colPlayer[]   PROGMEM = {255,   0,   0};   // red
const uint8_t colSolution[] PROGMEM = {255,   0, 255};   // magenta
const uint8_t colHint[]     PROGMEM = {  0, 255, 255};   // cyan
const uint8_t colVisited[]  PROGMEM = {128,   0, 128};   // purple
```

## Controls

### Movement Controls
- **Up Button (Pin 2)**: Move player upward through maze paths
- **Down Button (Pin 3)**: Move player downward through maze paths
- **Left Button (Pin 4)**: Move player left through maze paths  
- **Right Button (Pin 5)**: Move player right through maze paths

### Hint System Controls
- **Short Press Hint Button (Pin 7)**: Display next 3 optimal moves for 3 seconds
- **Long Press Hint Button (Pin 7)**: Show complete solution with animated progression
- **Second Hint Press**: Hide currently displayed hints or solution

### Start Screen
- **Any Movement Button**: Generate new maze and begin gameplay

## Game Rules

### Objective
Navigate the red player marker from the green start position to the yellow finish position using the shortest path possible.

### Movement Rules
- **Valid Movement**: Player can only move through black path areas
- **Wall Blocking**: Blue walls block all movement attempts
- **No Diagonal Movement**: Only up, down, left, right directions allowed
- **Boundary Limits**: Cannot move outside matrix edges

### Scoring & Progression
- **Move Counting**: Every successful movement increments move counter
- **Optimal Path Comparison**: Compare your performance against A* calculated optimal solution
- **Personal Best Tracking**: System remembers and displays your best move count
- **Infinite Levels**: Each completion generates progressively complex new maze

### Hint System Rules
- **Fair Assistance**: Hints show optimal next moves without penalty
- **Timed Display**: Short hints auto-hide after 3 seconds to encourage memory
- **Full Solution**: Long press reveals complete path for learning purposes
- **Context Sensitive**: Hints adapt based on current player position in solution

## Installation

### Libraries Required
```cpp
#include <FastLED.h>    // For WS2812B LED control
```

### Setup Steps
1. Install the **FastLED** library via Arduino IDE's Library Manager
2. Wire all components according to the wiring diagram above
3. Upload the `Maze_Runner.ino` sketch to your Arduino Nano
4. Open Serial Monitor (9600 baud) for level progress and debugging information
5. Press any movement button to generate first maze and start playing

## Troubleshooting

- **Maze Not Generating**: Check serial output for "Solution exists" confirmation. Verify random seed initialization.
- **Movement Not Working**: Confirm button wiring and pull-up resistors. Code uses `INPUT_PULLUP` for internal pull-ups.
- **Hints Not Displaying**: Verify hint button wiring to Pin 7. Check serial output for hint activation messages.
- **Pathfinding Errors**: Ensure matrix dimensions match your hardware. Check A* algorithm initialization.
- **Performance Issues**: Large mazes may cause memory constraints on Arduino Uno. Consider Arduino Mega for complex mazes.

## Technical Features

### Recursive Backtracking Maze Generation
Creates perfect mazes with guaranteed single solution:
```cpp
void createMazeRecursive(uint8_t x, uint8_t y) {
  maze[y][x] = PATH;
  // Shuffle directions randomly
  // Recursively carve paths in valid directions
}
```

### A* Pathfinding Implementation
Industry-standard pathfinding for optimal solution calculation:
```cpp
struct Coord {                     // solution points – only coordinates
  uint8_t x, y;
};
Coord solutionPath[MAX_PATH_LENGTH];

struct OpenNode {                  // nodes kept in the open set
  uint8_t x, y, g;                 // g = cost from start
};
OpenNode openSet[MAX_OPEN_SET];
```

### Smart Hint System
Context-aware assistance based on player position:
```cpp
// Find player position in solution path
int playerPosInSolution = -1;
for (int i = 0; i < solutionLength; i++) {
  if (solutionPath[i].x == playerX && solutionPath[i].y == playerY) {
    playerPosInSolution = i;
    break;
  }
}
```

### Memory-Efficient Design
Optimized data structures for Arduino constraints:
```cpp
// Compact maze storage using #define constants
#define WALL   0
#define PATH   1
#define START  2
#define FINISH 3
uint8_t maze[ROWS][COLS];
```

---

**Hardware**: Arduino Nano + 160 WS2812B LEDs + 5 Buttons  
**Difficulty**: Advanced Arduino Project