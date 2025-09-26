# Arduino LED Matrix Library Collection

A comprehensive library of Arduino LED projects featuring games, effects, and interactive applications for WS2812B LED matrices and strips.

![Collection](https://img.shields.io/badge/Projects-7_Complete-brightgreen) ![Hardware](https://img.shields.io/badge/Hardware-Arduino_Nano-blue) ![LEDs](https://img.shields.io/badge/LEDs-WS2812B-orange)

> [!NOTE]
> This is just the beginning of my experimentation—many more projects are on the way! I plan to add detailed wiring diagrams and pictures for all projects to better showcase the concepts and final designs.

## 🎮 Project Collection

### **Interactive Games**
| Project | Description | LEDs | Features |
|---|---|---|---|
| **[Pong Game](./Interactive%20Games/README_Pong_Game.md)** | Two-player Pong with dynamic ball physics | 220 (10×22) | Smoothed controls, rally speed, visual feedback |
| **[Dino Runner](./Interactive%20Games/README_Dino_Game.md)** | Chrome Dino-inspired endless runner | 198 (9×22) | Jump/duck, particles, advanced physics |

### **Creative Tools**
| Project | Description | LEDs | Features |  
|---|---|---|---|
| **[Drawing Machine](./Creative%20Tools/README_Drawing_Machine.md)** | Interactive drawing with potentiometers | 220 (10×22) | Color mixing, pen modes, canvas clear |

### **Visual Effects**
| Project | Description | LEDs | Features |
|---|---|---|---|
| **[Music Visualizer](./Visual%20Effects/README_Music_Visualizer.md)** | Real-time audio spectrum analyzer | 220 (10×22) | FFT analysis, 7-bar visualizer, mic input |
| **[Effects Playlist](./Visual%20Effects/README_LED_Effects_Playlist.md)** | 8 professional effects with auto-cycle | 198 (9×22) | Fireworks, fire, rainbow, particles |
| **[Liquid Cooler](./Visual%20Effects/README_Liquid_Cooler.md)** | PC cooling simulation with temperature | 20 (linear) | Multi-zone, thermal response, warnings |

### **Drivers & Utilities**
| Project | Description | LEDs | Features |
|---|---|---|---|
| **[Matrix Driver](./Drivers%20&%20Utilities/README_MatrixDriver.md)** | Serial-controlled driver for matrices | 198 (9×22) | Serial protocol, full-frame updates, brightness control |


## 🔧 Hardware Requirements

### **Common Components**
- **Arduino Nano** (recommended) or compatible microcontroller
- **WS2812B LEDs** (various configurations)
- **5V Power Supply** (capacity depends on LED count)
- **Capacitors** (470µF-1000µF for power stability)
- **Resistors** (330Ω for data line, 10kΩ for pull-ups)

### **Matrix Configurations**
```
Pong Game:        10×22 = 220 LEDs (serpentine)
Drawing Machine:  10×22 = 220 LEDs (serpentine)
Music Visualizer: 10×22 = 220 LEDs (serpentine)
Dino Game:         9×22 = 198 LEDs (serpentine)
Effects Playlist:  9×22 = 198 LEDs (serpentine)
Matrix Driver:     9×22 = 198 LEDs (serpentine)
Liquid Cooler:    20 LEDs (linear strip)
```

### **Input Hardware**
- **Potentiometers**: 10kΩ linear for analog control
- **Buttons**: Momentary push buttons with pull-up resistors
- **Microphone**: MAX4466 or similar for audio input

## 📚 Documentation

Each project includes comprehensive documentation:

- **Hardware Requirements**: Complete component lists and wiring
- **Feature Descriptions**: Detailed functionality explanations  
- **Installation Guides**: Step-by-step setup instructions
- **Configuration Options**: Customizable parameters and settings
- **Troubleshooting**: Common issues and solutions
- **Customization Ideas**: Enhancement suggestions and modifications

## 🚀 Quick Start Guide

### **1. Choose Your Project**
Select based on your hardware and interests:
- **Gaming**: Pong or Dino Runner for interactive entertainment
- **Art**: Drawing Machine for creative expression  
- **Display**: Music Visualizer or Effects Playlist for a dynamic show
- **PC Modding**: Liquid Cooler for system monitoring
- **Development**: Matrix Driver to create your own PC-controlled display

### **2. Hardware Setup**
```
Basic Wiring (All Projects):
Arduino Pin 6 -> LED Data Line  
5V -> LED VCC + Component Power
GND -> LED GND + Component Ground
```

### **3. Software Installation**  
```
1. Install Arduino IDE
2. Install required libraries (e.g., FastLED) via Library Manager
3. Select Arduino Nano as target board
4. Upload chosen project sketch
```

### **4. Power Considerations**
```
LED Count -> Recommended Power Supply:
20 LEDs   -> 5V 2A
198 LEDs  -> 5V 8A  
220 LEDs  -> 5V 10A
```

## 🎯 Project Complexity Levels

### **Beginner Friendly**
- **Effects Playlist**: No user input, autonomous operation
- **Liquid Cooler**: Simple setup, automatic temperature simulation

### **Intermediate**
- **Drawing Machine**: Multi-input coordination, real-time interaction
- **Pong Game**: Game logic, collision detection, scoring
- **Music Visualizer**: FFT and audio processing

### **Advanced**  
- **Dino Game**: Complex physics, particle systems, AI features
- **Matrix Driver**: Requires a separate program to send serial commands

## 🛠️ Common Code Patterns

### **Matrix Mapping (Serpentine)**
```cpp
int XY(int x, int y) {
  if (y % 2 == 0) {
    return y * COLS + x;
  } else {  
    return y * COLS + (COLS - 1 - x);
  }
}
```

### **Button Debouncing**
```cpp
if (reading != lastButtonState) {
  lastDebounceTime = millis();
}
if ((millis() - lastDebounceTime) > debounceDelay) {
  if (reading != buttonState) {
    buttonState = reading;
    if (buttonState == LOW) {
      // Button press action
    }
  }
}
```

### **Smooth Analog Input**
```cpp
float smoothValue = (alpha * rawValue) + ((1 - alpha) * smoothValue);
```

## 🤝 Contributing

### **Adding New Projects**
1. Follow existing code structure and documentation standards
2. Include comprehensive README with hardware requirements
3. Test on actual hardware before submission
4. Optimize for Arduino Nano memory constraints

### **Improvement Suggestions**
- Code optimizations for better performance
- Additional visual effects and animations  
- Enhanced user interface features
- Hardware integration examples
- Power efficiency improvements

---

**🎉 Complete Arduino LED Library**  
*Professional-quality projects with comprehensive documentation*

**Perfect for**: Learning, prototyping, art installations, gaming, PC modding  
**Skill Range**: Beginner to Advanced  
**Result**: Stunning LED displays with interactive functionality*