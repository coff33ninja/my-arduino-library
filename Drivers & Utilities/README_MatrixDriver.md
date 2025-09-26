# Arduino LED Matrix Driver

A GPIO-only driver for WS2812B LED matrices that receives commands over serial to control the display. This sketch is optimized for performance and can be used as a standalone driver for various LED matrix projects.

![Status](https://img.shields.io/badge/Status-Complete-brightgreen) ![Features](https://img.shields.io/badge/Features-Advanced-blue)

## Features

### 🚀 **High-Performance Serial Protocol**
- **Optimized for Speed**: Baud rate of 115200 for fast data transfer.
- **Multiple Commands**: Control individual pixels, full frames, brightness, and more.
- **Efficient Data Handling**: Non-blocking serial reception.

### 💡 **Versatile LED Control**
- **Full Frame Updates**: Send a complete frame of pixel data to the matrix.
- **Single Pixel Updates**: Update individual pixels for small changes.
- **Global Brightness Control**: Adjust the brightness of the entire matrix on the fly.
- **Fill and Clear**: Fill the entire matrix with a single color or clear it to black.

### ⚙️ **System Commands**
- **Status Reporting**: Request status information from the driver, including uptime, frame count, and more.
- **Test Pattern**: Display a rainbow test pattern to verify hardware setup.
- **Version Info**: Request the protocol version for compatibility checking.

### 🔌 **Hardware & Layout**
- **Flexible Configuration**: Easily change the matrix dimensions, LED pin, and other settings.
- **Serpentine Layout**: Supports serpentine (zigzag) matrix layouts.
- **Power Management**: Default brightness limiting for power efficiency.

## Hardware Requirements

### Components
- **Arduino Nano** (or compatible microcontroller)
- **WS2812B LED Matrix** (e.g., 10x22, 220 LEDs total)
- **Power Supply** - 5V, with sufficient amperage for your matrix (e.g., 10A+ for 198 LEDs at full brightness)
- **Capacitor** - 1000µF across the power supply for stability

### Wiring Diagram
```
Arduino Nano -> Component
=====================================
Pin 6        -> LED Matrix Data Line
5V           -> LED Matrix VCC
GND          -> LED Matrix GND
```

### Matrix Configuration
- **Dimensions**: Configurable (default: 22 columns × 9 rows = 198 LEDs)
- **Layout**: Serpentine wiring (zigzag pattern)
- **Origin**: Top-left corner

## Serial Protocol

The driver communicates over serial at a baud rate of **115200**. Commands are sent as a sequence of bytes.

### Commands

| Command | Byte | Parameters | Description |
|---|---|---|---|
| `CMD_FRAME` | `0xFF` | `R,G,B, R,G,B, ...` | Update the entire frame. Expects `ROWS * COLS * 3` bytes. |
| `CMD_PIXEL` | `0x01` | `x, y, R, G, B` | Set a single pixel at the given coordinates to the given color. |
| `CMD_BRIGHT` | `0x02` | `brightness` | Set the global brightness of the matrix (0-255). |
| `CMD_CLEAR` | `0x03` | - | Clear the entire matrix to black. |
| `CMD_FILL` | `0x04` | `R, G, B` | Fill the entire matrix with the given color. |
| `CMD_STATUS` | `0x05` | - | Request status information from the driver. |
| `CMD_TEST` | `0x06` | - | Run a rainbow test pattern. |
| `CMD_VERSION` | `0x07` | - | Request the protocol version. |

## Installation & Setup

### Arduino IDE Setup
1. **Install FastLED Library**: Go to `Tools` > `Manage Libraries` and search for "FastLED".
2. **Board Selection**: Go to `Tools` > `Board` and select "Arduino Nano".
3. **Port Selection**: Go to `Tools` > `Port` and select your Arduino's port.
4. **Compile & Upload**: Verify the code compiles, then upload it to your Arduino.

### First Run
1. **Power Connection**: Connect the 5V power supply to the matrix.
2. **Serial Monitor**: Open the Serial Monitor at a baud rate of 115200 to see the driver's status messages.
3. **Test Commands**: Send commands over serial to test the driver.

## Troubleshooting

- **No Display**: Check the power supply capacity and the LED data connection.
- **Wrong Colors**: Verify the `LED_TYPE` (e.g., `WS2812B`) and `COLOR_ORDER` (e.g., `GRB`) in the code.
- **Flickering**: Add a 1000µF capacitor across the power supply.
- **Unresponsive**: Check the serial connection and baud rate.

## Customization Ideas

### Easy Modifications
- **Matrix Size**: Change the `ROWS` and `COLS` defines to match your matrix.
- **LED Pin**: Change the `LED_PIN` define to use a different pin.
- **Default Brightness**: Change the `DEFAULT_BRIGHTNESS` define to set a different default brightness.

### Advanced Enhancements
- **Add More Commands**: Extend the serial protocol with new commands for animations, text scrolling, or other features.
- **EEPROM Storage**: Store the brightness setting in EEPROM so it persists after a power cycle.
- **Add a Watchdog**: Implement a watchdog timer to reset the Arduino if it becomes unresponsive.
