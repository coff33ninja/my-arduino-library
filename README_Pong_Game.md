# Arduino LED Matrix Pong Game

A feature-rich Pong game for WS2812B LED matrices, enhanced with smooth controls, progressive difficulty, and polished visual feedback.

![Pong Game Demo](https://img.shields.io/badge/Status-Complete-brightgreen) ![Platform](https://img.shields.io/badge/Platform-Arduino-blue)

## Features

### 🎮 **Gameplay Mechanics**
- **Two-Player Action**: Classic head-to-head paddle gameplay.
- **Angled Bounces**: Ball direction changes based on where it hits the paddle.
- **Rally-Based Speed Increase**: Ball speed increases after every 4-hit rally, creating a progressive difficulty curve.
- **Advanced LED Scoreboard**: Top-row scoreboard supports scores up to 109 using a color-coded system (units and tens).
- **Configurable Winning Score**: The game ends when a player reaches the `WINNING_SCORE` (default: 10).
- **Win Animation**: A celebratory flashing animation plays for the winning player.

### 🌟 **Visual Effects & Feedback**
- **Paddle Hit Flash**: Paddles flash white on ball impact for clear visual feedback.
- **Dynamic Brightness**: The matrix brightness increases with the combined score of both players.
- **Power-Safe Brightness**: Maximum brightness is capped to a safe level to protect hardware.
- **Ball Trails**: A fade effect creates a trail behind the moving ball.
- **Score & Reset Animations**: A rainbow wave plays when the ball resets, and the screen flashes on a score.

### 🕹️ **Controls & Input**
- **Smoothed Paddle Control**: Exponential smoothing on potentiometer inputs eliminates jitter for precise, responsive movement.
- **Debounced Reset Button**: A physical button allows for instant game resets without electrical noise issues.

## Hardware Requirements

### Components
- **Arduino Nano** (or compatible microcontroller)
- **10x22 WS2812B LED Matrix** (220 LEDs total)
- **2x 10kΩ Potentiometers** for paddle control
- **1x Push Button** for game reset
- **Power Supply**: 5V with sufficient amperage for 220 LEDs (e.g., 5V 2A+ recommended).
- **Capacitor**: 1000µF across the power lines to stabilize voltage.
- **Resistor**: 330-470Ω on the LED data line to protect the first pixel.

### Wiring Diagram
```
Arduino Nano -> Component
=====================================
Pin 6        -> LED Matrix Data (via 330Ω resistor)
Pin A0       -> Left Paddle Pot (middle pin)
Pin A1       -> Right Paddle Pot (middle pin)
Pin 2        -> Reset Button (other pin to GND)
5V           -> LED Matrix VCC, Potentiometers VCC
GND          -> LED Matrix GND, Potentiometers GND, Button GND
A3           -> Floating (used for randomSeed)
```

## Configuration Options

The following parameters can be easily customized at the top of the `.ino` file.

### Gameplay Settings
```cpp
#define PADDLE_HEIGHT 3           // Height of paddles in LEDs
#define BALL_SPEED_MIN 40         // Fastest ball speed (ms delay)
#define BALL_SPEED_START 100      // Starting ball speed (ms delay)
#define WINNING_SCORE 10          // Score needed to win the match
#define BOUNCE_ANGLES 1           // 1=angled bounces, 0=straight
```

### Display & Matrix Settings
```cpp
#define MATRIX_WIDTH 22           // Width of the LED matrix
#define MATRIX_HEIGHT 10          // Height of the LED matrix
#define FADE_AMOUNT 40            // Fade rate for ball trails (0-255)
#define SCOREBOARD_ENABLED 1      // 1=LED scores, 0=Serial output
```

## Controls

### During Game
- **Left Potentiometer**: Controls the vertical position of the left (blue) paddle.
- **Right Potentiometer**: Controls the vertical position of the right (green) paddle.
- **Reset Button**: A short press immediately restarts the game.

## Game Rules

### Scoring
- If the ball passes the left edge, the right player scores a point.
- If the ball passes the right edge, the left player scores a point.
- The first player to reach the `WINNING_SCORE` wins the game.
- The scoreboard uses different colors for tens and units:
  - **Player 1**: Blue (units), Red (tens)
  - **Player 2**: Green (units), Yellow (tens)

### Ball Physics
- **Speed**: The ball starts at `BALL_SPEED_START` and gets faster every 4-hit rally.
- **Bounces**: The ball bounces off the top and bottom walls.
- **Angles**: If `BOUNCE_ANGLES` is enabled, the ball's vertical direction will change based on where it strikes a paddle (top, middle, or bottom).

## Installation

### Libraries Required
```cpp
#include <FastLED.h>    // For LED control
#include <stdlib.h>     // For random() function
```

### Setup Steps
1. Install the **FastLED** library via the Arduino IDE's Library Manager.
2. Wire the components according to the wiring diagram.
3. Upload the `Pong_Game.ino` sketch to your Arduino Nano.
4. Turn the potentiometers to ensure the paddles move smoothly.
5. Press the reset button to start a new game.

## Troubleshooting

- **LEDs Flicker or Show Wrong Colors**: Ensure your power supply is adequate and shares a common ground with the Arduino. Check the `GRB` color order in the `FastLED.addLeds` line.
- **Paddles are Jittery**: The code now includes smoothing. If jitter persists, check for loose potentiometer connections.
- **Game Doesn't Reset**: Ensure your reset button is wired correctly between Pin 2 and GND. The code uses `INPUT_PULLUP`, so no external resistor is needed.

## Technical Features

### Input Smoothing
To ensure stable paddle control, the raw analog readings from the potentiometers are filtered using an exponential moving average algorithm. This removes noise while maintaining responsiveness.
```cpp
// Smoothing factor (higher = more responsive)
const float alpha = 0.7;

// In loop():
smoothedPot1 = alpha * raw_pot_value + (1 - alpha) * smoothedPot1;
```

### Serpentine Matrix Mapping
The `getLEDIndex()` function correctly maps (x, y) coordinates to the linear index of a serpentine (or zigzag) wired matrix, ensuring the display is drawn correctly.

---

**Hardware**: Arduino Nano + 220 WS2812B LEDs + 2 Potentiometers + 1 Button  
**Difficulty**: Intermediate Arduino Project