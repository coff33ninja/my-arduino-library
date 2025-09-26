# Arduino LED Matrix Dino Runner Game

A Chrome Dino-inspired endless runner game for WS2812B LED matrices with enhanced graphics, physics, and gameplay mechanics.

![Dino Game Demo](https://img.shields.io/badge/Status-Complete-brightgreen) ![Features](https://img.shields.io/badge/Features-Advanced-blue)

## Game Features

### 🦕 **Enhanced Dino Character**
- **6 Animation Frames**: 4 running animations + jumping + ducking
- **Smooth Physics**: Realistic jumping with gravity and velocity
- **Multiple Actions**: Run, jump, duck to avoid obstacles
- **Visual Feedback**: Invincibility flashing, game over colors

### 🌵 **Dynamic Obstacle System**
- **Varied Obstacles**: Different heights and widths
- **Progressive Difficulty**: Obstacles get larger and more frequent
- **Smart Spawning**: Randomized timing prevents patterns
- **Gradient Effects**: Visual depth with color variations

### 🎮 **Advanced Gameplay**
- **Lives System**: 3 lives with heart indicators
- **Invincibility Frames**: Brief protection after hits
- **Speed Progression**: Game accelerates as score increases
- **Smart Collision**: Ducking avoids tall obstacles, jumping clears all

### 🌟 **Visual Effects**
- **Twinkling Stars**: Dynamic night sky background
- **Animated Ground**: Moving texture effects
- **UI Elements**: Score bars, speed indicators, lives display
- **Power Management**: Brightness control and power limiting

## Hardware Requirements

### Components
- **Arduino Nano** (or compatible microcontroller)
- **10x22 WS2812B LED Matrix** (220 LEDs total)
- **Jump Button** - Pin 2 (with pull-up resistor)
- **Duck Button** - Pin 3 (optional, with pull-up resistor)  
- **Power Supply** - 5V, 10A+ recommended for full brightness
- **Capacitor** - 1000µF across power supply for stability

### Wiring Diagram
```
Arduino Nano -> Component
=====================================
Pin 6        -> LED Matrix Data Line
Pin 2        -> Jump Button (other end to GND)
Pin 3        -> Duck Button (other end to GND)
5V           -> LED Matrix VCC + Button Pull-ups
GND          -> LED Matrix GND + Button Commons
A0           -> Floating (used for randomSeed)
```

### Matrix Configuration
- **Dimensions**: 22 columns × 9 rows = 198 LEDs
- **Layout**: Serpentine wiring (zigzag pattern)
- **Origin**: Bottom-left corner
- **Data Flow**: Row 0 (bottom) flows right-to-left, Row 1 flows left-to-right, etc.

## Controls

### Button Layout
- **Pin 2 (Jump Button)**: Press to make dino jump over obstacles
- **Pin 3 (Duck Button)**: Hold to make dino duck under tall obstacles
- **Game Over**: Press jump button to restart immediately

### Control Tips
- **Timing**: Jump just before obstacles reach the dino
- **Ducking**: Hold duck button for tall obstacles (3+ pixels high)  
- **Strategy**: Some obstacles can be avoided by either jumping OR ducking
- **Rhythm**: Find the timing pattern as speed increases

## Game Mechanics

### Scoring System
- **Points**: Continuous scoring based on distance
- **Speed Bonus**: Higher speeds give more points per frame
- **Progression**: Every 1000 points increases game speed (max 5x)
- **Display**: Yellow score bar fills across top row

### Lives & Health
- **Starting Lives**: 3 lives (shown as red hearts)
- **Hit Penalty**: Lose 1 life on collision, brief invincibility period
- **Game Over**: All lives lost triggers game over state
- **Recovery**: Nearby obstacles removed after hit for brief relief

### Physics Engine
- **Jump Velocity**: 2 pixels/frame initial upward speed
- **Gravity**: 1 pixel/frame^2 downward acceleration
- **Max Height**: 3 pixels above ground level
- **Landing**: Automatic ground detection and landing

### Obstacle Behavior
- **Movement**: Obstacles move left at current game speed
- **Spawning**: Random intervals with minimum safe distances
- **Variety**: Heights from 2-5 pixels, widths from 1-3 pixels
- **Cleanup**: Off-screen obstacles automatically removed

## Visual System

### Color Scheme
```cpp
Dino:     Lime Green (jumping), White (invincible), Red (game over)
Obstacles: Forest Green with gradient effects
Ground:    Saddle Brown with animated texture
Sky:       Midnight Blue (dimmed to 25%)
Stars:     Cool colors with individual twinkling
UI:        Red hearts, Blue speed, Yellow score
```

### Animation System
- **Frame Rate**: Variable based on game speed (50-150ms)
- **Smooth Transitions**: Exponential easing between frames
- **State-Based**: Different animations for run/jump/duck states
- **Performance**: PROGMEM storage for sprite data efficiency

### Star Field Effect
- **Count**: 6 strategically placed twinkling stars
- **Behavior**: 3-second twinkle cycles with individual timing
- **Colors**: Random cool tones (blues, purples, whites)
- **Brightness**: Very dim to avoid overwhelming gameplay

## Advanced Features

### AI Assistant (Optional)
```cpp
// Uncomment in updateGame() for AI help:
if (!game.isJumping && !game.invincible && shouldJump()) {
  startJump();
}
```
- **Smart Detection**: Analyzes incoming obstacles
- **Timing Calculation**: Predicts optimal jump moments
- **Height Analysis**: Only jumps for obstacles requiring it
- **Manual Override**: Player input always takes priority

### Power Management
- **Brightness Limiting**: Default 32/255 for power efficiency
- **Voltage/Current Limits**: 5V @ 1000mA maximum
- **Dynamic Brightness**: Dims during game over state
- **LED Count Optimization**: Efficient serpentine mapping

### Debug Features
```cpp
// Uncomment testMatrix() in setup() for LED testing
void testMatrix() {
  // Cycles through each row with rainbow colors
  // Helps verify wiring and coordinate mapping
}
```

## Configuration Options

### Game Tuning
```cpp
#define MAX_JUMP_HEIGHT 3      // Maximum jump height
#define JUMP_VELOCITY 2        // Initial jump speed
#define GRAVITY 1              // Downward acceleration
#define MAX_OBSTACLES 8        // Maximum active obstacles
#define NUM_STARS 6            // Background star count
```

### Display Settings
```cpp
#define BRIGHTNESS 32          // LED brightness (0-255)
#define ROWS 9                 // Matrix height
#define COLS 22                // Matrix width  
#define LED_PIN 6              // Data pin
#define NUM_LEDS 198           // Total LED count
```

## Installation & Setup

### Arduino IDE Setup
1. **Install FastLED Library**: Tools → Manage Libraries → Search \"FastLED\"
2. **Board Selection**: Tools → Board → Arduino Nano
3. **Port Selection**: Tools → Port → Select your Arduino's port
4. **Compile & Upload**: Verify code compiles, then upload

### First Run
1. **Power Connection**: Connect 5V power supply to matrix
2. **Serial Monitor**: Open at 9600 baud to see game status
3. **Test Controls**: Press buttons to verify input response
4. **Calibration**: Adjust brightness if needed for your setup

### Troubleshooting
- **No Display**: Check power supply capacity and LED data connection
- **Wrong Colors**: Verify LED_TYPE (WS2812B) and COLOR_ORDER (GRB)
- **Flickering**: Add 1000µF capacitor across power supply
- **Unresponsive**: Check button pull-up resistors and connections

## Customization Ideas

### Easy Modifications
- **Colors**: Change DINO_COLOR, CACTUS_COLOR in defines
- **Speed**: Adjust JUMP_VELOCITY and GRAVITY for different feel
- **Difficulty**: Modify spawn delays and obstacle sizes
- **Lives**: Change starting life count in initializeGame()

### Advanced Enhancements
- **Sound Effects**: Add buzzer on pin for jump/hit sounds
- **High Scores**: Store best score in EEPROM memory
- **Power-ups**: Add special items that give advantages
- **Different Obstacles**: Create flying enemies that require ducking
- **Weather Effects**: Add rain or wind visual effects

### Matrix Size Adaptation
```cpp
// For different matrix sizes:
#define ROWS 16        // Your matrix height
#define COLS 32        // Your matrix width
#define NUM_LEDS (ROWS * COLS)  // Update total count
```

## Performance Notes

### Memory Usage
- **Program Space**: ~75% of Arduino Nano flash memory
- **SRAM**: ~60% of available RAM
- **PROGMEM**: Sprite data stored in flash to save RAM
- **Optimization**: Efficient data structures and minimal allocations

### Frame Rate
- **Target**: 20-60 FPS depending on game speed
- **Adaptive**: Slower speeds use longer frame times
- **Smooth**: No blocking delays in main loop
- **Responsive**: Input handling optimized for low latency

---

**🎮 Ready to Play!**  
*Jump over cacti, duck under obstacles, beat your high score!*

**Hardware**: Arduino Nano + 198 WS2812B LEDs + 2 Buttons  
**Difficulty**: Intermediate Arduino project  
**Play Time**: Endless (until you lose all lives!)*
