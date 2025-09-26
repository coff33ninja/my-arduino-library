# Arduino LED Matrix Drawing Machine

An interactive drawing application for WS2812B LED matrices with smooth cursor control and line drawing capabilities.

![Drawing Machine Demo](https://img.shields.io/badge/Status-Functional-brightgreen)

## Features

### Drawing Capabilities
- **Smooth cursor movement** with exponential smoothing
- **Pen up/down modes** - draw or just move cursor
- **Line drawing** - Bresenham algorithm for clean lines
- **Color control** - Full HSV color wheel selection
- **Canvas clearing** - Long-press to erase artwork

### Visual Effects
- **Color preview** - See current color when pen is up
- **Smooth blending** - Preview overlays existing artwork
- **Real-time feedback** - Instant response to input changes
- **Trail-free drawing** - Clean, precise line rendering

### User Interface
- **Intuitive controls** - Three potentiometers + one button
- **Visual feedback** - Different modes clearly indicated
- **Responsive input** - Low-latency cursor tracking
- **Error prevention** - Boundary checking prevents crashes

## Hardware Requirements

### Components
- **Arduino Nano** (or compatible)
- **10x22 WS2812B LED Matrix** (220 LEDs total) 
- **3x 10kΩ Potentiometers** for X, Y, and Color control
- **1x Push Button** for pen control and canvas clearing
- **Power Supply** - 5V, adequate for 220 LEDs
- **Resistors** - 330Ω for LED data line, 10kΩ pull-up for button

### Wiring
```
Arduino Nano -> Component
=====================================
Pin 6        -> LED Matrix Data
Pin A0       -> X Position Pot (middle pin)
Pin A1       -> Y Position Pot (middle pin) 
Pin A2       -> Color Selection Pot (middle pin)
Pin 2        -> Button (other pin to GND)
5V           -> LED Matrix VCC, All Pot VCC
GND          -> LED Matrix GND, All Pot GND, Button GND
```

### Matrix Layout
- **Dimensions**: 22 wide × 10 high (configurable)
- **Wiring**: Serpentine (zigzag pattern)
- **LED Type**: WS2812B/NeoPixel compatible
- **Data Flow**: Configurable serpentine or standard

## Configuration Options

### Matrix Settings
```cpp
#define WIDTH 22              // Matrix width in pixels
#define HEIGHT 10             // Matrix height in pixels
#define DATA_PIN 6            // LED strip data pin
bool serpentine = true;       // true=zigzag, false=standard
```

### Input Settings
```cpp
#define POT_X_PIN A0          // X position potentiometer
#define POT_Y_PIN A1          // Y position potentiometer  
#define POT_COLOR_PIN A2      // Color selection potentiometer
#define BUTTON_PIN 2          // Pen control button
```

### Performance Settings
```cpp
uint8_t brightness = 64;      // LED brightness (0-255)
const float alpha = 0.15;     // Smoothing factor (0-1)
const unsigned long debounceDelay = 50;  // Button debounce (ms)
const unsigned long longPressDuration = 2000;  // Clear duration (ms)
```

## Controls

### Potentiometers
- **X-Axis Pot (A0)**: Controls horizontal cursor position
- **Y-Axis Pot (A1)**: Controls vertical cursor position
- **Color Pot (A2)**: Selects drawing color (full HSV spectrum)

### Button Functions
- **Short Press**: Toggle between pen-up and pen-down modes
  - Pen Up: Move cursor without drawing (preview mode)
  - Pen Down: Draw lines as you move cursor
- **Long Press (2+ seconds)**: Clear entire canvas
  - Visual confirmation with white flashes
  - Returns to pen-up mode after clearing

## Drawing Modes

### Pen-Up Mode (Preview)
- Cursor shows current selected color
- No permanent marks made on canvas
- Color blends with existing artwork for preview
- Move freely to position for next drawing

### Pen-Down Mode (Drawing)  
- Draws continuous lines between cursor positions
- Uses Bresenham line algorithm for smooth lines
- Full saturation colors for vibrant artwork
- Automatically connects previous position to current

## Visual Feedback

### Color System
- **HSV Color Space**: Full spectrum available via color pot
- **Real-time Preview**: See color changes immediately
- **Vibrant Output**: High saturation for bold artwork

### User Interface
- **Cursor Indication**: Current position always visible
- **Mode Awareness**: Clear difference between up/down modes
- **Smooth Movement**: Exponential smoothing prevents jitter
- **Boundary Safety**: Cursor cannot go outside matrix bounds

## Technical Features

### Smoothing Algorithm
```cpp
// Exponential smoothing for stable cursor movement
smoothX = (alpha * rawInput) + ((1 - alpha) * smoothX);
```
- Reduces potentiometer noise and jitter
- Configurable smoothing strength via `alpha`
- Maintains responsiveness while filtering noise

### Line Drawing
- **Bresenham Algorithm**: Efficient, accurate line rendering
- **Boundary Checking**: Prevents array overflow errors
- **Color Consistency**: Uniform color along entire line
- **Gap Prevention**: No missing pixels in diagonal lines

### Button Handling
- **Debouncing**: Eliminates contact bounce noise
- **Long Press Detection**: Time-based state machine
- **Visual Feedback**: Clear confirmation of actions
- **State Management**: Reliable mode switching

## Installation

### Libraries Required
```cpp
#include <FastLED.h>    // LED matrix control
```

### Setup Steps
1. Install FastLED library in Arduino IDE
2. Wire components according to wiring diagram
3. Upload sketch to Arduino Nano
4. Test cursor movement with X/Y potentiometers
5. Try color changes with color potentiometer
6. Test pen toggle with short button press
7. Test canvas clear with 2-second button hold

## Usage Instructions

### Getting Started
1. Power on the system
2. Cursor appears at center of matrix
3. Adjust X/Y pots to move cursor around
4. Turn color pot to see different colors
5. Short-press button to start drawing
6. Move cursor to draw colorful lines
7. Short-press button again to stop drawing
8. Hold button 2+ seconds to clear canvas

### Drawing Tips
- Start in pen-up mode to position cursor
- Use smooth, deliberate movements for clean lines
- Experiment with different colors for variety
- Clear canvas when starting new artwork
- Work in sections for detailed drawings

## Troubleshooting

### Common Issues
- **Jittery cursor**: Adjust `alpha` smoothing value
- **Wrong colors**: Check color potentiometer wiring
- **No drawing**: Verify pen-down mode is active
- **Button not working**: Check pull-up resistor/connection
- **LED issues**: Verify power supply and data connection

### Performance Optimization
- Lower brightness to reduce power consumption
- Increase smoothing for steadier cursor
- Adjust loop delay for responsiveness balance
- Use adequate power supply for full brightness

## Customization Ideas

### Easy Modifications
- Change matrix dimensions in `WIDTH`/`HEIGHT`
- Adjust cursor smoothing with `alpha` value
- Modify colors in HSV color space
- Add different brush sizes or shapes
- Create preset color palettes

### Advanced Features
- **Save/Load**: Store drawings in EEPROM
- **Animation**: Record and playback drawing sequences
- **Multi-brush**: Different drawing tools and effects
- **Network**: Share drawings between devices
- **Sound**: Audio feedback for drawing actions

## Code Structure

### Key Functions
- `XY_to_index()` - Converts coordinates to LED array index
- `readPots()` - Handles input smoothing and mapping
- `drawLine()` - Bresenham line drawing with bounds checking
- `handleButton()` - Debounced button with long-press detection
- `clearCanvas()` - Canvas clearing with visual feedback
- `render()` - Main drawing and preview rendering

### Main Loop Flow
1. Read and smooth potentiometer inputs
2. Handle button presses and long-press detection
3. Render current frame (drawing or preview)
4. Update LED matrix display
5. Small delay for stable operation
6. Repeat continuously

### Memory Management
- **Efficient Storage**: Single LED array for entire matrix
- **Real-time Processing**: No frame buffering needed
- **Minimal RAM**: Optimized for Arduino Nano constraints

---

**Created for Arduino Nano + WS2812B LED Matrix**  
*Supports any rectangular matrix size with configurable serpentine wiring*