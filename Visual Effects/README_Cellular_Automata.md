# Arduino LED Matrix Cellular Automata

A comprehensive cellular automata simulator for WS2812B LED matrices featuring multiple rule systems, interactive controls, and educational demonstrations of complex emergent behaviors.

![Cellular Automata Demo](https://img.shields.io/badge/Status-In%20Progress-yellow) ![Platform](https://img.shields.io/badge/Platform-Arduino-blue)

## Features

### 🧬 **Multiple Automata Rules**
- **Conway's Game of Life**: Classic B3/S23 rule showcasing gliders, oscillators, and stable patterns
- **Rule 30**: Elementary cellular automaton demonstrating chaotic behavior and pseudo-randomness
- **Rule 110**: Turing-complete elementary CA capable of universal computation
- **Brian's Brain**: Three-state automaton creating beautiful pulsing and spreading patterns
- **Seeds**: Explosive rule B2/S creating rapid expansion and decay patterns
- **Life Without Death**: Accumulating variant where cells never die, only grow
- **Maze**: Specialized rule that generates organic maze-like structures
- **Coral Growth**: Crystal-formation rule creating branch-like growth patterns

### 🎮 **Interactive Control System**
- **Rule Switching**: Cycle through all 8 automata rules during runtime without restart
- **Grid Reset**: Instantly clear and reseed current rule with appropriate pattern
- **Speed Control**: Four-level speed adjustment (Slow/Medium/Fast/Ultra) for optimal viewing
- **Real-Time Changes**: Modify parameters while simulation runs for immediate feedback
- **Pattern Seeding**: Each rule starts with scientifically appropriate initial conditions

### 🌟 **Visual & Educational Features**
- **Rule-Specific Color Schemes**: Each automata uses distinct color palette for visual distinction
- **Multi-State Cell Support**: Beyond simple alive/dead, supports dying, excited, and custom states
- **Generation Milestones**: Visual feedback every 100 generations to track long-term evolution
- **Famous Pattern Library**: Pre-programmed gliders, pulsars, beacons for Game of Life
- **Toroidal Topology**: Wrapping edges create continuous world without boundaries

### 🔬 **Scientific Accuracy**
- **Correct Rule Implementation**: Each CA follows precise mathematical definitions from literature
- **Proper Neighborhood Calculations**: Accurate Moore neighborhood (8-cell) counting for 2D rules
- **Elementary CA Support**: Correct 1D rule processing for Rule 30 and Rule 110
- **State Transition Accuracy**: Faithful reproduction of published cellular automata behaviors

## Hardware Requirements

### Components
- **Arduino Nano** (or compatible microcontroller)
- **10x22 WS2812B LED Matrix** (220 LEDs total)
- **3x Push Buttons** for interactive control
- **3x 10kΩ Pull-up Resistors** for button stability
- **Power Supply**: 5V with sufficient amperage for 220 LEDs (recommended: 5V 10A)
- **Capacitor**: 470µF-1000µF across power lines for voltage stabilization
- **Resistor**: 330Ω on LED data line for signal protection

### Wiring Diagram
```
Arduino Nano -> Component
=====================================
Pin 6        -> LED Matrix Data (via 330Ω resistor)
Pin 2        -> Next Rule Button (other pin to GND)
Pin 3        -> Reset Button (other pin to GND)
Pin 4        -> Speed Toggle Button (other pin to GND)
5V           -> LED Matrix VCC
GND          -> LED Matrix GND, All Button Commons
A0           -> Floating (used for randomSeed)
A1           -> Floating (used for additional entropy)
```

## Memory Usage

**Warning:** This sketch consumes a significant amount of dynamic memory (RAM), using approximately 1953 bytes (95%) on a standard Arduino Nano. This leaves very little memory for local variables, which may lead to stability issues. I will optimize it to fit and work better on Uno and Nano devices, sometime, as I havent tested them yet.

- **Sketch Storage:** 9300 bytes (30%)
- **Global Variables:** 1953 bytes (95%)
- **Remaining RAM:** 95 bytes

Due to the high RAM usage, an **ESP32 or other microcontroller with more RAM is highly recommended** for a stable experience.

## Configuration Options

The following parameters can be easily customized at the top of the `.ino` file.

### Timing Settings
```cpp
#define SPEED_SLOW 500           // Slow update interval (ms)
#define SPEED_MEDIUM 200         // Medium update interval (ms)  
#define SPEED_FAST 100           // Fast update interval (ms)
#define SPEED_ULTRA 50           // Ultra-fast update interval (ms)
```

### Visual Settings
```cpp
#define BRIGHTNESS 80            // LED brightness (0-255)
#define COLS 10                  // Matrix width in LEDs
#define ROWS 22                  // Matrix height in LEDs
```

### Cellular Automata States
```cpp
#define DEAD 0                   // Standard dead cell state
#define ALIVE 1                  // Standard alive cell state  
#define DYING 2                  // Intermediate state (Brian's Brain)
#define EXCITED 3                // Special state for complex rules
```

## Controls

### Runtime Controls
- **Next Rule Button (Pin 2)**: Cycle through all 8 cellular automata rules
- **Reset Button (Pin 3)**: Clear grid and reseed with rule-appropriate pattern
- **Speed Button (Pin 4)**: Toggle between Slow → Medium → Fast → Ultra speeds

### Automatic Features
- **Rule Auto-Seeding**: Each rule automatically receives appropriate starting pattern
- **Serial Feedback**: Rule names and speed changes reported via serial monitor
- **Generation Tracking**: Automatic generation counting with milestone indicators

## Cellular Automata Rules

### Two-Dimensional Rules

#### Conway's Game of Life (B3/S23)
- **Birth**: Dead cell with exactly 3 neighbors becomes alive
- **Survival**: Live cell with 2 or 3 neighbors stays alive
- **Death**: All other conditions result in cell death
- **Patterns**: Features famous gliders, pulsars, and beacon configurations

#### Brian's Brain
- **Three States**: Dead → Alive → Dying → Dead cycle
- **Birth Rule**: Dead cell with exactly 2 alive neighbors becomes alive
- **Death Rule**: All alive cells become dying next generation
- **Visual**: Creates pulsing, wave-like propagation patterns

#### Seeds (B2/S)
- **Explosive Growth**: Dead cells with 2 neighbors become alive
- **No Survival**: All alive cells die immediately next generation
- **Behavior**: Rapid expansion followed by complete decay

#### Life Without Death
- **Birth Only**: Dead cells with 3 neighbors become alive
- **Immortality**: Alive cells never die
- **Growth**: Continuously accumulating patterns

#### Maze Rule
- **Birth**: Dead cells with exactly 3 neighbors become alive
- **Survival**: Live cells with 1-5 neighbors stay alive
- **Pattern**: Creates organic maze-like corridor structures

#### Coral Growth
- **Persistent Growth**: Live cells never die (coral doesn't decay)
- **Growth Condition**: Dead cells with 3+ neighbors become alive
- **Behavior**: Branch-like crystal formation resembling coral reefs

### One-Dimensional Elementary Rules

#### Rule 30
- **Chaotic Behavior**: Produces pseudo-random patterns from simple initial conditions
- **Initial State**: Single alive cell at top center
- **Rule Binary**: 00011110 - defines neighborhood transition rules
- **Applications**: Used in Wolfram's random number generators

#### Rule 110
- **Turing Complete**: Capable of universal computation
- **Complex Patterns**: Shows both regular and chaotic behaviors
- **Rule Binary**: 01101110 - creates intricate propagating structures
- **Scientific Importance**: Demonstrates computation in simple systems

## Pattern Library

### Game of Life Patterns
- **Glider**: 3×3 pattern that moves diagonally across the grid
- **Pulsar**: 5×5 oscillating pattern with period-3 behavior
- **Beacon**: 4×4 oscillator alternating between two states

### Custom Seeding
Each rule receives appropriate initial patterns:
- **Random Noise**: Seeds, Brian's Brain use scattered random cells
- **Single Point**: Rule 30, Rule 110 start with single center cell
- **Structured Patterns**: Game of Life uses famous configurations
- **Growth Points**: Coral Growth starts with small seed cluster

## Installation

### Libraries Required
```cpp
#include <FastLED.h>    // For WS2812B LED control
```

### Setup Steps
1. Install the **FastLED** library via Arduino IDE's Library Manager
2. Wire all components according to the wiring diagram above
3. Upload the `Cellular_Automata.ino` sketch to your Arduino Nano
4. Open Serial Monitor (9600 baud) for rule information and generation tracking
5. Use control buttons to explore different cellular automata rules

## Troubleshooting

- **Simulation Not Starting**: Check serial output for initialization messages. Verify power supply capacity.
- **Buttons Not Responding**: Confirm button wiring and pull-up resistors. Code uses `INPUT_PULLUP` for internal pull-ups.
- **Pattern Not Evolving**: Some rules may reach stable states. Press reset button to generate new initial conditions.
- **Memory Issues**: Complex rules may challenge Arduino Uno memory. Consider Arduino Mega for extended features.
- **Speed Too Fast/Slow**: Use speed button to adjust or modify timing constants in code.

## Educational Applications

### Computer Science Concepts
- **Emergence**: Simple rules creating complex behaviors
- **Computational Theory**: Turing completeness demonstration
- **Algorithm Visualization**: State transitions and rule applications
- **Chaos Theory**: Deterministic systems with unpredictable outcomes

### Mathematical Concepts
- **Discrete Dynamics**: Step-by-step system evolution
- **Neighborhood Functions**: Spatial relationship calculations
- **Boolean Logic**: Binary state transitions and rule encoding
- **Probability**: Random initial conditions and pattern emergence

### Scientific Applications
- **Biology**: Population dynamics and ecosystem modeling
- **Physics**: Phase transitions and critical phenomena
- **Chemistry**: Reaction-diffusion systems and pattern formation
- **Artificial Life**: Self-organization and evolutionary processes

## Technical Features

### Efficient Memory Management
Dual-grid system for smooth state transitions:
```cpp
uint8_t currentGrid[ROWS][COLS];
uint8_t nextGrid[ROWS][COLS];
// Calculate next generation, then swap grids
```

### Toroidal Boundary Conditions
Wrapping edges create continuous world:
```cpp
// Wrap around edges for seamless topology
if (nx < 0) nx = COLS - 1;
if (nx >= COLS) nx = 0;
if (ny < 0) ny = ROWS - 1;
if (ny >= ROWS) ny = 0;
```

### Rule-Specific Color Systems
Visual distinction between different automata:
```cpp
// Each rule has unique 4-color palette
CRGB ruleColors[TOTAL_RULES][4] = {
  {CRGB::Black, CRGB::White, CRGB::Red, CRGB::Blue},    // Game of Life
  {CRGB::Black, CRGB::Yellow, CRGB::Orange, CRGB::Red}, // Rule 30
  // ... additional color schemes for each rule
};
```

### Pattern Placement System
Structured initial condition setup:
```cpp
void placePattern(Pattern& pattern, int startX, int startY) {
  for (int y = 0; y < pattern.height; y++) {
    for (int x = 0; x < pattern.width; x++) {
      // Place pattern data at specified grid location
    }
  }
}
```

## Advanced Customization

### Adding New Rules
1. Increment `TOTAL_RULES` constant
2. Add rule name to `ruleNames[]` array
3. Add color scheme to `ruleColors[]` array
4. Implement update function following existing pattern
5. Add case to `updateAutomata()` switch statement
6. Create appropriate seeding function

### Custom Color Schemes
Modify `ruleColors` array for different visual themes:
```cpp
// Example: High contrast scheme
{CRGB::Black, CRGB::White, CRGB::Red, CRGB::Blue}
// Example: Warm color scheme  
{CRGB::Black, CRGB::Orange, CRGB::Red, CRGB::Yellow}
```

---

**Hardware**: Arduino Nano + 220 WS2812B LEDs + 3 Buttons  
**Difficulty**: Advanced Arduino Project  
**Educational Value**: High - Demonstrates emergence, chaos theory, and computational concepts