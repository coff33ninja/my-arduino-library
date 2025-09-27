# Arduino LED Matrix Maze Runner

A procedurally generated maze game for WS2812B LED matrices featuring advanced pathfinding AI, intelligent hint systems, and infinite level progression.

![Maze Runner Demo](https://img.shields.io/badge/Status-In%20Progress-yellow) ![Platform](https://img.shields.io/badge/Platform-Arduino-blue)

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
- **10x22 WS2812B LED Matrix** (220 LEDs total)
- **5x Push Buttons** for directional control and hints
- **5x 10kΩ Pull-up Resistors** for button stability
- **Power Supply**: 5V with sufficient amperage for 220 LEDs (recommended: 5V 10A)
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

**Warning:** This sketch consumes a significant amount of dynamic memory (RAM), using approximately 1988 bytes (97%) on a standard Arduino Nano. This leaves very little memory for local variables, which may lead to stability issues. I will optimize it to fit and work better on Uno and Nano devices, sometime, as I havent tested them yet.

- **Sketch Storage:** 11298 bytes (36%)
- **Global Variables:** 1988 bytes (97%)
- **Remaining RAM:** 60 bytes

Due to the high RAM usage, an **ESP32 or other microcontroller with more RAM is highly recommended** for a stable experience.

## Configuration Options

The following parameters can be easily customized at the top of the `.ino` file.

### Maze Generation Settings
```cpp
#define WALL_DENSITY 0.3         // Lower = more open paths (0.0-1.0)
#define MAX_PATH_LENGTH 100      // Maximum solution path length
```

### Visual & Timing Settings
```cpp
#define HINT_DURATION 3000       // Hint display time (ms)
#define SOLUTION_SPEED 200       // Solution animation speed (ms/step)
#define BRIGHTNESS 100           // LED brightness (0-255)
```

### Color Customization
```cpp
#define COLOR_WALL CRGB::Blue        // Maze wall color
#define COLOR_PLAYER CRGB::Red       // Player marker color
#define COLOR_START CRGB::Green      // Start position color
#define COLOR_FINISH CRGB::Yellow    // Finish position color
#define COLOR_HINT CRGB::Cyan        // Hint path color
#define COLOR_SOLUTION CRGB::Magenta // Solution path color
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
void createMazeRecursive(int x, int y) {
  maze[y][x] = PATH;
  // Shuffle directions randomly
  // Recursively carve paths in valid directions
}
```

### A* Pathfinding Implementation
Industry-standard pathfinding for optimal solution calculation:
```cpp
struct Point {
  int x, y;
  int f, g, h;  // A* algorithm values
  Point* parent; // For path reconstruction
};
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
// Compact maze storage using enum cell types
enum CellType {
  WALL = 0, PATH = 1, START = 2, FINISH = 3
};
CellType maze[ROWS][COLS];
```

---

**Hardware**: Arduino Nano + 220 WS2812B LEDs + 5 Buttons  
**Difficulty**: Advanced Arduino Project