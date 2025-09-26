# Arduino LED Effects Playlist

A comprehensive collection of 8 stunning LED effects that automatically cycle through a visual playlist on WS2812B matrices.

![Effects Demo](https://img.shields.io/badge/Effects-8_Amazing-rainbow) ![Status](https://img.shields.io/badge/Status-Production_Ready-brightgreen)

## Features

### 🎨 **8 Professional Effects**
1. **Rainbow Cycle** - Classic scrolling rainbow across entire matrix
2. **Color Chase** - Single pixel chases around matrix with color cycling
3. **Comet Effect** - Bright comet with fading trail
4. **Glitter Effect** - Random white sparkles on fading background
5. **Juggle Effect** - Multiple colored dots with sine wave motion
6. **Fire Effect** - Realistic fire simulation with heat diffusion
7. **Fireworks Effect** - Full particle physics with rockets and sparks
8. **Diagonal Rainbow** - Geometric rainbow patterns with smooth transitions

### 🚀 **Advanced Technical Features**
- **Automatic Playlist**: Effects cycle automatically after completion
- **Optimized Performance**: Precomputed sine/cosine lookup tables
- **Particle Physics**: Realistic fireworks with gravity and momentum
- **Memory Efficient**: PROGMEM storage for lookup tables
- **Smooth Animation**: Variable timing for different effect speeds
- **Professional Code**: Modular function pointer architecture

## Hardware Requirements

### Components
- **Arduino Nano** (or compatible)
- **10x22 WS2812B LED Matrix** (220 LEDs)
- **Power Supply** - 5V, 10A+ for full brightness
- **Capacitor** - 1000µF for power stability

### Wiring
```
Arduino -> LED Matrix
====================
Pin 6   -> Data Line
5V      -> VCC
GND     -> GND
```

### Matrix Configuration
- **Layout**: Serpentine (zigzag) wiring
- **Origin**: Bottom-left corner
- **Flow**: Even rows left→right, odd rows right→left

## Effect Details

### 🌈 **Rainbow Cycle**
- **Duration**: 5.12 seconds per cycle
- **Speed**: 20ms frame rate
- **Pattern**: HSV color wheel mapped across all LEDs
- **Animation**: Continuous color rotation

### 🏃 **Color Chase**
- **Duration**: ~10 seconds per complete cycle
- **Speed**: 50ms frame rate  
- **Pattern**: Single pixel travels through all LEDs
- **Colors**: Continuously shifting hue

### ☄️ **Comet Effect**
- **Duration**: ~12 seconds (2 complete orbits)
- **Speed**: 30ms frame rate
- **Trail**: Fading tail with 40-unit fade per frame
- **Colors**: Time-based hue shifting

### ✨ **Glitter Effect**
- **Duration**: 8 seconds
- **Speed**: 40ms frame rate
- **Sparkles**: 80/256 chance per frame per LED
- **Background**: Slow 20-unit fade creates ambience

### 🤹 **Juggle Effect** 
- **Duration**: ~33 seconds
- **Speed**: 15ms frame rate
- **Dots**: 8 colored dots with individual sine wave motion
- **Pattern**: BeatSin16 creates smooth organic movement

### 🔥 **Fire Effect**
- **Duration**: 6 seconds
- **Speed**: 30ms frame rate  
- **Physics**: Heat diffusion with cooling and random ignition
- **Colors**: HeatColor palette (black→red→orange→yellow→white)

### 🎆 **Fireworks Effect**
- **Duration**: Continuous (no auto-advance)
- **Components**: Rockets (5 max) + Sparks (40 max)
- **Physics**: Gravity, velocity, particle lifetime
- **Colors**: Random hues with slight variations

### 🌈 **Diagonal Rainbow**
- **Duration**: Continuous with fade cycles
- **Speed**: 30ms frame rate
- **Pattern**: Diagonal color bands across matrix
- **Animation**: Smooth corner transitions with beatsin8 pulse

## Technical Implementation

### Performance Optimizations
```cpp
// Precomputed lookup tables save CPU cycles
const int8_t PROGMEM cosTable[32] = {...};
const int8_t PROGMEM sinTable[32] = {...};

// Efficient serpentine mapping
static uint16_t XY(uint8_t x, uint8_t y) {
  return (y * COLS) + ((y & 1) ? (COLS - 1 - x) : x);
}
```

### Memory Management
- **Flash Usage**: ~75% of Arduino Nano (efficient PROGMEM usage)
- **SRAM Usage**: ~40% (optimized data structures)
- **Dynamic Allocation**: Minimal, mostly static arrays

### Particle System (Fireworks)
```cpp
struct Rocket {
  int16_t x, y;     // Position
  int8_t vx, vy;    // Velocity
  uint8_t hue;      // Color
  bool active;      // State
};

struct Spark {
  int16_t x, y;     // Position  
  int8_t vx, vy;    // Velocity
  uint8_t hue;      // Color
  uint8_t life;     // Remaining lifetime
  bool active;      // State
};
```

## Configuration Options

### Matrix Settings
```cpp
#define ROWS 9           // Matrix height
#define COLS 22          // Matrix width
#define NUM_LEDS 198     // Total LED count
#define LED_PIN 6        // Data pin
#define BRIGHTNESS 32    // Global brightness (0-255)
```

### Effect Timing
```cpp
// Modify effect durations by changing these values:
EVERY_N_MILLISECONDS(20) { /* Rainbow speed */ }
EVERY_N_MILLISECONDS(50) { /* Chase speed */ }
EVERY_N_MILLISECONDS(30) { /* Comet speed */ }
```

### Particle Limits
```cpp
#define MAX_ROCKETS 5    // Firework rockets
#define MAX_SPARKS 40    // Spark particles
#define NUM_ANGLES 32    // Lookup table size
```

## Installation

### Arduino IDE Setup
1. **Install FastLED**: Tools → Library Manager → "FastLED"
2. **Board**: Arduino Nano
3. **Processor**: ATmega328P (Old Bootloader if needed)
4. **Upload**: Compile and flash to Arduino

### Power Considerations
- **5V @ 10A**: Recommended for full brightness
- **Current Limiting**: FastLED automatically handles power
- **Brightness**: Start at 32/255, increase as power allows
- **Capacitor**: 1000µF across power supply prevents voltage drops

## Usage

### Operation
1. **Power On**: Effects playlist starts automatically
2. **Auto-Cycle**: Each effect runs to completion then advances
3. **Continuous**: Playlist loops indefinitely
4. **No Input**: Fully autonomous operation

### Customization
```cpp
// Add your own effects:
bool myCustomEffect() {
  EVERY_N_MILLISECONDS(50) {
    // Your effect code here
    FastLED.show();
    tickCounter++;
  }
  return (tickCounter > 100); // End condition
}

// Add to effects array:
EffectFunc effects[] = {
  rainbowCycle,
  myCustomEffect,  // <-- Your effect
  colorChase,
  // etc...
};
```

## Troubleshooting

### Common Issues
- **No Display**: Check power supply and wiring
- **Wrong Colors**: Verify COLOR_ORDER (GRB vs RGB)
- **Flickering**: Add power supply capacitor
- **Dim LEDs**: Increase BRIGHTNESS value
- **Memory Issues**: Reduce particle counts if needed

### Performance Tips
- **Lower Brightness**: Reduces power consumption
- **Fewer Particles**: Reduces CPU load for fireworks
- **Slower Timing**: Increase millisecond delays
- **Power Budget**: Monitor voltage under load

## Advanced Features

### Effect Function Architecture
```cpp
typedef bool (*EffectFunc)();  // Function pointer type
EffectFunc effects[];          // Array of effect functions
bool effectComplete = effects[currentEffect](); // Call current effect
```

### Automatic Advancement
```cpp
void nextEffect() {
  currentEffect = (currentEffect + 1) % numEffects;
  effectRound = 0;     // Reset completion counter
  tickCounter = 0;     // Reset frame counter
  fill_solid(leds, NUM_LEDS, CRGB::Black); // Clear display
}
```

### State Management
- **tickCounter**: Tracks animation frames within effects
- **effectRound**: Counts completed cycles for auto-advance
- **currentEffect**: Index of active effect in playlist

---

**🎭 Professional LED Effects System**  
*8 stunning effects, automatic playlist, production-ready code*

**Perfect for**: Art installations, mood lighting, displays, demos  
**Skill Level**: Intermediate to Advanced  
**Result**: Mesmerizing continuous light show*