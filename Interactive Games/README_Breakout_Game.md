# Arduino LED Matrix Breakout Game

A comprehensive Breakout/Arkanoid game for WS2812B LED matrices featuring power-ups, smooth ball physics, and progressive difficulty levels.

![Breakout Game Demo](https://img.shields.io/badge/Status-In%20Progress-yellow) ![Platform](https://img.shields.io/badge/Platform-Arduino-blue)

## Features

### 🎮 **Gameplay Mechanics**
- **Classic Breakout Action**: Control paddle to bounce ball and destroy colored brick layers.
- **Advanced Ball Physics**: Realistic collision detection with paddle-based english and spin effects.
- **Progressive Levels**: Brick difficulty increases with level progression, requiring multiple hits to destroy.
- **Lives System**: Start with 3 lives, lose one when ball reaches bottom, gain lives through power-ups.
- **Scoring System**: Different colored bricks award different point values (10-50 points per brick).
- **Multi-Ball Support**: Simultaneous multiple balls during multiball power-up phase.

### 🌟 **Power-Up System**
- **Wide Paddle**: Increases paddle size from 3 to 5 LEDs for easier ball control.
- **Fast Ball**: Doubles ball speed for rapid gameplay and higher challenge.
- **Multiball**: Spawns additional balls for increased destruction potential.
- **Extra Life**: Grants additional life to extend gameplay.
- **Timed Duration**: All power-ups last 10 seconds before expiring.
- **Visual Indicators**: Power-ups displayed in distinct colors while falling.

### 🎨 **Visual Effects & Feedback**
- **Color-Coded Bricks**: Six distinct brick colors representing different point values and positions.
- **Damaged Brick States**: Multi-hit bricks darken when damaged to show progress.
- **Animated Start Screen**: Rainbow sweep animation creates engaging startup experience.
- **Level Completion Celebration**: Rainbow explosion effect emanating from screen center.
- **Game Over Animation**: Dramatic cascade fall effect followed by score visualization.
- **Lives Display**: Bottom row shows remaining lives as red indicator LEDs.

### 🕹️ **Dual Control Options**
- **Button Control**: Discrete left/right movement using physical push buttons.
- **Potentiometer Control**: Smooth analog paddle positioning for precise control.
- **Configurable**: Switch between control modes via `USE_POTENTIOMETER` setting.
- **Responsive Input**: Optimized input timing prevents lag during fast gameplay.

## Hardware Requirements

### Components
- **Arduino Nano** (or compatible microcontroller)
- **10x22 WS2812B LED Matrix** (220 LEDs total)
- **Control Option A - Buttons**: 2x Push buttons + 2x 10kΩ pull-up resistors
- **Control Option B - Potentiometer**: 1x 10kΩ linear potentiometer
- **Power Supply**: 5V with sufficient amperage for 220 LEDs (recommended: 5V 10A)
- **Capacitor**: 470µF-1000µF across power lines for voltage stabilization
- **Resistor**: 330Ω on LED data line for signal protection

### Wiring Diagram
```
Arduino Nano -> Component
=====================================
Pin 6        -> LED Matrix Data (via 330Ω resistor)

Button Control Option:
Pin 2        -> Left Button (other pin to GND)
Pin 3        -> Right Button (other pin to GND)

Potentiometer Control Option:
Pin A0       -> Potentiometer Center Tap
5V           -> Potentiometer One End
GND          -> Potentiometer Other End

Common Connections:
5V           -> LED Matrix VCC
GND          -> LED Matrix GND
A1           -> Floating (used for randomSeed)
```

## Memory Usage

**Warning:** This sketch consumes a significant amount of dynamic memory (RAM), using approximately 1875 bytes (91%) on a standard Arduino Nano. This leaves very little memory for local variables, which may lead to stability issues. I will optimize it to fit and work better on Uno and Nano devices, sometime, as I havent tested them yet.

- **Sketch Storage:** 11662 bytes (37%)
- **Global Variables:** 1875 bytes (91%)
- **Remaining RAM:** 173 bytes

Due to the high RAM usage, an **ESP32 or other microcontroller with more RAM is highly recommended** for a stable experience.

## Configuration Options

The following parameters can be easily customized at the top of the `.ino` file.

### Control Settings
```cpp
#define USE_POTENTIOMETER true   // true=analog control, false=button control
#define PADDLE_WIDTH 3           // Default paddle size (3 LEDs)
#define PADDLE_SPEED 80          // Button control responsiveness (ms)
```

### Gameplay Settings
```cpp
#define BALL_SPEED 120           // Base ball movement delay (ms)
#define POWERUP_CHANCE 20        // Power-up spawn probability (%)
#define POWERUP_DURATION 10000   // Power-up effect duration (ms)
#define BRICK_ROWS 6             // Number of brick rows
```

### Display Settings
```cpp
#define BRIGHTNESS 100           // LED brightness (0-255)
#define COLS 10                  // Matrix width in LEDs
#define ROWS 22                  // Matrix height in LEDs
```

## Controls

### Button Control Mode (`USE_POTENTIOMETER = false`)
- **Left Button (Pin 2)**: Move paddle left
- **Right Button (Pin 3)**: Move paddle right

### Potentiometer Control Mode (`USE_POTENTIOMETER = true`)
- **Potentiometer**: Smooth analog paddle positioning across full matrix width

### Universal Controls
- **Any Control Input**: Start game from animated start screen

## Game Rules

### Objective
Destroy all colored bricks by bouncing the ball with your paddle while keeping the ball from falling off the bottom of the screen.

### Brick Values & Colors
- **Red Bricks** (Top Row): 50 points each
- **Orange Bricks**: 40 points each
- **Yellow Bricks**: 30 points each  
- **Green Bricks**: 20 points each
- **Blue Bricks**: 15 points each
- **Purple Bricks** (Bottom Row): 10 points each

### Ball Physics
- **Paddle English**: Ball angle changes based on where it hits the paddle (left/center/right)
- **Wall Bouncing**: Ball bounces off left, right, and top walls
- **Brick Destruction**: Ball reverses vertical direction when hitting bricks
- **Multi-Hit Bricks**: Higher levels require multiple hits, shown by darkening colors

### Power-Up Mechanics
- **Spawn Rate**: 20% chance per brick destroyed
- **Collection**: Catch falling power-ups with paddle to activate
- **Duration**: Effects last 10 seconds unless otherwise specified
- **Stacking**: Multiple power-ups can be active simultaneously

## Installation

### Libraries Required
```cpp
#include <FastLED.h>    // For WS2812B LED control
```

### Setup Steps
1. Install the **FastLED** library via Arduino IDE's Library Manager
2. Choose control method by setting `USE_POTENTIOMETER` constant
3. Wire components according to chosen control scheme
4. Upload the `Breakout_Game.ino` sketch to your Arduino Nano
5. Open Serial Monitor (9600 baud) for score tracking and debugging
6. Use control input to start playing

## Troubleshooting

- **LEDs Flickering**: Ensure adequate power supply and proper grounding. Check LED_TYPE and COLOR_ORDER settings.
- **Paddle Not Moving Smoothly**: For potentiometer mode, check wiring and analog connections. For button mode, verify pull-up resistors.
- **Ball Physics Issues**: Verify matrix dimensions match your hardware. Check serpentine wiring configuration.
- **Power-ups Not Appearing**: Confirm POWERUP_CHANCE setting and verify random seed initialization.
- **Game Too Easy/Hard**: Adjust BALL_SPEED, POWERUP_CHANCE, or brick hit requirements.

## Technical Features

### Advanced Ball Physics
Realistic paddle interaction with english effects:
```cpp
// Add spin based on paddle hit position
float hitPos = (float)(ballX - paddlePos) / paddleWidth;
ball.dx = (hitPos - 0.5) * 1.6;  // -0.8 to 0.8 range
```

### Power-Up Management System
Efficient power-up spawning and tracking:
```cpp
// Spawn power-up with probability check
if (random(100) < POWERUP_CHANCE) {
  spawnPowerUp(ballX, brickRow);
}
```

### Multi-Ball Implementation
Support for simultaneous multiple balls:
```cpp
// Promote extra ball to main ball when main is lost
if (extraBallCount > 0) {
  ball = extraBalls[0];
  // Shift remaining extra balls...
}
```

### Level Progression
Dynamic brick difficulty scaling:
```cpp
// Higher levels require multiple hits
if (level > 3) {
  bricks[row][col].hits = 2;
  bricks[row][col].color = bricks[row][col].color % 128; // Darker
}
```

---

**Hardware**: Arduino Nano + 220 WS2812B LEDs + 2 Buttons OR 1 Potentiometer  
**Difficulty**: Advanced Arduino Project