# Arduino Liquid Cooler LED Simulation

A realistic PC liquid cooling system visualization with temperature-responsive effects, dynamic fan speeds, and thermal management simulation for WS2812B LED strips.

![Cooler Demo](https://img.shields.io/badge/Simulation-Realistic-blue) ![Status](https://img.shields.io/badge/Status-Enhanced-brightgreen)

## Features

### 🌡️ **Temperature Simulation**
- **Dynamic Temperature**: Simulates CPU temperatures from 25-75°C
- **Smooth Transitions**: Gradual temperature changes for realistic behavior
- **Overheating Protection**: Warning effects when temperature exceeds 70°C
- **Serial Monitoring**: Real-time temperature readouts

### 💨 **Multi-Zone Cooling**
- **Fan 1**: 6 LEDs, clockwise rotation
- **Fan 2**: 6 LEDs, counter-clockwise rotation
- **Pump**: 8 LEDs, slower rotation for liquid flow effect
- **Independent Control**: Each zone has different speeds and directions

### 🎨 **Temperature-Responsive Colors**
- **Cool (25-40°C)**: Deep blue → Purple transition
- **Warm (40-60°C)**: Purple → Orange transition  
- **Hot (60-75°C)**: Orange → Red transition
- **Overheating (>70°C)**: Pulsing red warning + increased intensity

### ⚡ **Dynamic Performance**
- **Speed Scaling**: Fan/pump speeds increase with temperature
- **Visual Intensity**: Brighter effects during overheating
- **Realistic Physics**: Spinning motion with trailing blur effects
- **60fps Updates**: Smooth animation for fluid motion

## Hardware Requirements

### Components
- **Arduino Nano** (or compatible)
- **20 WS2812B LEDs** arranged as:
  - Fan 1: 6 LEDs (positions 0-5)
  - Fan 2: 6 LEDs (positions 6-11)
  - Pump: 8 LEDs (positions 12-19)
- **Power Supply**: 5V, 2A minimum
- **Capacitor**: 470µF for power stability

### Wiring
```
Arduino -> LED Strip
====================
Pin 3   -> Data Line
5V      -> VCC  
GND     -> GND
```

### LED Layout
```
Fan 1 (LEDs 0-5)    Fan 2 (LEDs 6-11)    Pump (LEDs 12-19)
     0                    6                   12
   5   1                11  7               19  13
  4     2              10    8             18    14
   3                    9                 17  15
                                           16
```

## Temperature System

### Temperature Ranges
```cpp
25-40°C: Cool Operation
  - Colors: Blue to Purple
  - Speed: 1.0x base speed
  - Fade: Normal (40 units)

40-60°C: Warm Operation  
  - Colors: Purple to Orange
  - Speed: 1.5x base speed
  - Fade: Normal (40 units)

60-70°C: Hot Operation
  - Colors: Orange to Red
  - Speed: 2.0x base speed
  - Fade: Normal (40 units)

70+°C: Overheating
  - Colors: Bright Red
  - Speed: 3.0x base speed
  - Fade: Reduced (20 units) for intensity
  - Warning: Flashing red overlay
```

### Color Algorithm
```cpp
CRGB getCoolantColor(float temp) {
  if (temp < 40) {
    return blend(CRGB(0,0,255), CRGB(64,0,128), 
                 map(temp, 20, 40, 0, 255));
  } else if (temp < 60) {
    return blend(CRGB(64,0,128), CRGB(255,100,0), 
                 map(temp, 40, 60, 0, 255));  
  } else {
    return blend(CRGB(255,100,0), CRGB(255,0,0), 
                 map(temp, 60, 80, 0, 255));
  }
}
```

## Animation System

### Spinning Physics
- **Position Calculation**: `pos = time * speed * ledCount`
- **Wraparound**: Seamless circular motion
- **Trail Effect**: 4-LED trailing blur for motion blur
- **Direction Control**: Positive/negative speeds for rotation direction

### Trail Rendering
```cpp
for (int t = 0; t < trailLength; t++) {
  float ledPos = pos - t;  // Trail position
  uint8_t brightness = 255 - (t * (255/trailLength)); // Fade
  // Render with temperature-based colors
}
```

### Speed Calculation
```cpp
float speedMultiplier = map(temperature, 20, 80, 1.0, 3.0);
float fan1Speed = baseFan1Speed * speedMultiplier;  // 0.6 → 1.8 RPS
float fan2Speed = baseFan2Speed * speedMultiplier;  // -0.8 → -2.4 RPS  
float pumpSpeed = basePumpSpeed * speedMultiplier;  // 0.4 → 1.2 RPS
```

## Configuration Options

### Base Speeds (Rotations Per Second)
```cpp
float baseFan1Speed = 0.6;   // Fan 1 clockwise
float baseFan2Speed = -0.8;  // Fan 2 counter-clockwise
float basePumpSpeed = 0.4;   // Pump clockwise (slower)
```

### Visual Settings
```cpp
int trailLength = 4;         // Motion blur trail length
uint8_t brightness = 255;    // Maximum brightness
int fadeAmount = 40;         // Normal fade amount
int overheatFade = 20;       // Reduced fade when overheating
```

### Temperature Simulation
```cpp
float temperature = 30.0;    // Starting temperature
float targetTemp = 30.0;     // Target temperature
unsigned long tempInterval = 5000;  // 5 second temp changes
```

## Installation & Setup

### Arduino IDE
1. **Install FastLED Library**: Library Manager → FastLED
2. **Select Board**: Arduino Nano  
3. **Select Port**: Your Arduino's COM port
4. **Upload**: Compile and flash

### Serial Monitor
- **Baud Rate**: 9600
- **Output**: Temperature changes and system status
- **Example**: "Target temperature: 45.2°C"

### First Run
1. Connect power and upload code
2. Open Serial Monitor to see temperature simulation  
3. Watch LEDs change color and speed as temperature varies
4. Observe overheating warning when temp > 70°C

## Advanced Features

### Temperature Simulation
```cpp
// Every 5 seconds, pick new target temperature
if (currentMillis - lastTempUpdate >= 5000) {
  targetTemp = random(25, 75);  // 25-75°C range
  Serial.print("Target temperature: ");
  Serial.println(targetTemp);
}

// Smooth temperature transition
if (temperature < targetTemp) {
  temperature += 0.1;  // Heat up slowly
} else if (temperature > targetTemp) {
  temperature -= 0.1;  // Cool down slowly
}
```

### Overheating Protection
```cpp
overheating = (temperature > 70);

// Visual warning
if (overheating && (millis() / 200) % 2) {
  fill_solid(leds, NUM_LEDS, CRGB(255, 0, 0));
}

// Increased performance  
uint8_t fadeAmount = overheating ? 20 : 40;
if (temp > 70) {
  brightness = min(255, brightness * 1.5);
}
```

## Customization Ideas

### Easy Modifications
```cpp
// Change temperature ranges
#define COOL_TEMP 35      // Cool/warm threshold
#define WARM_TEMP 55      // Warm/hot threshold  
#define OVERHEAT_TEMP 65  // Overheating threshold

// Adjust rotation speeds
baseFan1Speed = 1.0;      // Faster fans
basePumpSpeed = 0.2;      // Slower pump

// Modify colors
CRGB coolColor = CRGB::Cyan;
CRGB hotColor = CRGB::OrangeRed;
```

### Advanced Enhancements
- **Temperature Sensor**: Replace simulation with real DS18B20 sensor
- **PWM Control**: Actually control real fans based on temperature
- **Web Interface**: WiFi module for remote monitoring
- **Sound Effects**: Buzzer for overheating alarms
- **Multiple Systems**: Monitor CPU, GPU, motherboard separately

## Real Hardware Integration

### Temperature Sensor (DS18B20)
```cpp
#include <OneWire.h>
#include <DallasTemperature.h>

OneWire oneWire(A0);
DallasTemperature sensors(&oneWire);

void setup() {
  sensors.begin();
}

float readRealTemperature() {
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}
```

### Fan Control (PWM)
```cpp
#define FAN1_PWM_PIN 9
#define FAN2_PWM_PIN 10
#define PUMP_PWM_PIN 11

void controlRealFans(float temp) {
  int fanSpeed = map(temp, 25, 75, 100, 255);  // PWM value
  analogWrite(FAN1_PWM_PIN, fanSpeed);
  analogWrite(FAN2_PWM_PIN, fanSpeed);
  analogWrite(PUMP_PWM_PIN, fanSpeed * 0.8);   // Pump slower
}
```

## Troubleshooting

### Common Issues
- **No LEDs**: Check power supply and data connection
- **Wrong Colors**: Verify WS2812B type and GRB color order
- **Jerky Motion**: Ensure adequate power supply
- **No Serial**: Check baud rate (9600) and cable connection

### Performance Tips
- **Lower Brightness**: Reduces power consumption
- **Fewer Trails**: Reduce `trailLength` for simpler animation  
- **Slower Updates**: Increase frame time for lower CPU usage
- **Power Budget**: Use 5V supply with adequate current capacity

## Technical Details

### Memory Usage
- **Program Space**: ~45% of Arduino Nano
- **SRAM**: ~25% of available RAM  
- **No Dynamic Allocation**: All arrays statically allocated
- **Efficient**: Float calculations optimized for Arduino

### Performance
- **Frame Rate**: 60fps (16.67ms per frame)
- **Temperature Update**: Every 5 seconds
- **Smooth Motion**: Sub-pixel positioning for fluid rotation
- **Real-time**: No blocking delays in main loop

### Color Science
- **Temperature Mapping**: Linear interpolation between color points
- **Brightness Control**: Multiplicative scaling preserves hue
- **Trail Fadeout**: Linear brightness reduction along trail
- **Color Blending**: FastLED's built-in blend() function

---

**🔧 Realistic PC Cooling Simulation**  
*Temperature-responsive LED effects with dynamic performance scaling*

**Perfect for**: PC case lighting, system monitoring, cooling demos  
**Hardware**: 20 WS2812B LEDs + Arduino Nano  
**Skill Level**: Intermediate  
**Result**: Professional cooling system visualization*