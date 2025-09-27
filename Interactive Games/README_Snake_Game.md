# Arduino LED Matrix Snake Game

A feature-rich Snake game for WS2812B LED matrices with growing tail mechanics, progressive difficulty, and polished visual feedback.

![Snake Game Demo](https://img.shields.io/badge/Status-In%20Progress-yellow) ![Platform](https://img.shields.io/badge/Platform-Arduino-blue)

## Features

### 🎮 **Gameplay Mechanics**
- **Classic Snake Gameplay**: Navigate the snake to eat food and grow longer while avoiding walls and your own tail.
- **Growing Tail**: Snake increases in length by one segment for each food eaten.
- **Progressive Speed**: Game speed increases with each food consumed, creating escalating difficulty.
- **Anti-Reverse Logic**: Prevents the snake from immediately reversing into itself for fair gameplay.
- **Collision Detection**: Accurate detection for wall hits and self-collision with visual feedback.
- **Score Tracking**: Points awarded based on food consumed with serial output for debugging.

### 🌟 **Visual Effects & Feedback**
- **Distinct Snake Parts**: Head displayed in bright green (Lime), body in standard green for clear visual distinction.
- **Animated Start Screen**: Pulsing green pattern creates an engaging startup experience.
- **Game Over Animation**: Three-flash red animation followed by score visualization using blue LEDs.
- **Food Appearance**: Bright red food spawns in random valid locations.
- **Serial Feedback**: Real-time score updates and game state notifications via serial monitor.

### 🕹️ **Controls & Input**
- **Four-Directional Control**: Responsive button controls for Up, Down, Left, and Right movement.
- **Debounced Input**: Advanced debouncing prevents accidental double-presses and ensures reliable control.
- **Direction Validation**: Intelligent input handling prevents impossible direction changes.

## Hardware Requirements

### Components
- **Arduino Nano** (or compatible microcontroller)
- **10x22 WS2812B LED Matrix** (220 LEDs total)
- **4x Push Buttons** for directional control
- **4x 10kΩ Pull-up Resistors** for button stability
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
5V           -> LED Matrix VCC
GND          -> LED Matrix GND, All Button Commons
A0           -> Floating (used for randomSeed)
```

## Memory Usage

**CRITICAL WARNING:** This sketch uses nearly all available dynamic memory (RAM) on a standard Arduino Nano, consuming approximately 2043 bytes (99%). This leaves only 5 bytes for local variables, which will almost certainly cause stability problems, crashes, and unpredictable behavior.

- **Sketch Storage:** 7736 bytes (25%)
- **Global Variables:** 2043 bytes (99%)
- **Remaining RAM:** 5 bytes

Due to this extreme memory limitation, an **ESP32 or other microcontroller with significantly more RAM is STRONGLY recommended** to run this sketch reliably.

## Configuration Options

The following parameters can be easily customized at the top of the `.ino` file.

### Gameplay Settings
```cpp
#define INITIAL_SPEED 300         // Starting delay between moves (ms)
#define SPEED_INCREMENT 15        // Speed increase per food eaten
#define MIN_SPEED 100            // Fastest possible speed limit
#define BRIGHTNESS 80            // LED brightness (0-255)
```

### Display & Matrix Settings
```cpp
#define COLS 10                  // Matrix width in LEDs
#define ROWS 22                  // Matrix height in LEDs
#define COLOR_SNAKE CRGB::Green  // Snake body color
#define COLOR_SNAKE_HEAD CRGB::Lime // Snake head color (brighter)
#define COLOR_FOOD CRGB::Red     // Food color
```

## Controls

### During Game
- **Up Button (Pin 2)**: Move snake upward
- **Down Button (Pin 3)**: Move snake downward  
- **Left Button (Pin 4)**: Move snake left
- **Right Button (Pin 5)**: Move snake right

### Start Screen
- **Any Direction Button**: Start a new game from the animated start screen

## Game Rules

### Objective
Guide the snake to eat red food pellets while avoiding:
- **Wall Collisions**: Snake dies if it hits any matrix edge
- **Self-Collisions**: Snake dies if head touches any part of its body
- **Perfect Game**: Fill the entire matrix with snake segments to win

### Scoring & Progression
- **1 Point** per food pellet consumed
- **Speed Increase**: Game gets faster after each food eaten
- **Growth**: Snake length increases by one segment per food
- **Best Score Tracking**: Personal best displayed during game over

### Food Mechanics
- Food spawns randomly in valid locations (not occupied by snake)
- Snake must move head over food to consume it
- New food appears immediately after consumption

## Installation

### Libraries Required
```cpp
#include <FastLED.h>    // For WS2812B LED control
```

### Setup Steps
1. Install the **FastLED** library via Arduino IDE's Library Manager
2. Wire components according to the wiring diagram above
3. Upload the `Snake_Game.ino` sketch to your Arduino Nano
4. Open Serial Monitor (9600 baud) for score tracking and debugging
5. Press any directional button to start playing

## Troubleshooting

- **LEDs Not Lighting**: Check power supply capacity and wiring. Ensure common ground between Arduino and LED power supply.
- **Erratic Snake Movement**: Verify button wiring and pull-up resistors. The code uses `INPUT_PULLUP` so external resistors may not be needed.
- **Game Too Fast/Slow**: Adjust `INITIAL_SPEED`, `SPEED_INCREMENT`, or `MIN_SPEED` constants.
- **Food Not Appearing**: Check that food generation logic isn't blocked by a full snake (normal for very high scores).

## Technical Features

### Serpentine Matrix Mapping
The `XY()` function correctly maps (x, y) coordinates to linear LED indices for serpentine (zigzag) wired matrices:
```cpp
int XY(int x, int y) {
  if (y % 2 == 0) {
    return y * COLS + x;
  } else {
    return y * COLS + (COLS - 1 - x);
  }
}
```

### Smart Food Generation
Food placement algorithm ensures valid positions:
```cpp
// Prevents food spawning inside snake body
bool validPosition = false;
while (!validPosition) {
  food.x = random(COLS);
  food.y = random(ROWS);
  // Check against all snake segments...
}
```

### Input Debouncing
Advanced button handling prevents input errors:
```cpp
if (digitalRead(BTN_UP) == LOW && 
    currentTime - lastButtonPress[0] > debounceDelay) {
  if (currentDirection != DOWN) {  // Anti-reverse logic
    nextDirection = UP;
  }
}
```

---

**Hardware**: Arduino Nano + 220 WS2812B LEDs + 4 Buttons  
**Difficulty**: Intermediate Arduino Project

**Hardware**: Arduino Nano + 220 WS2812B LEDs + 4 Buttons  
**Difficulty**: Intermediate Arduino Project