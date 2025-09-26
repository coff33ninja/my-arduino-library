# Sharingan Eye Animation for LED Matrix

A mesmerizing animation of the Sharingan eye from Naruto for WS2812B LED matrices.

![Sharingan Demo](https://img.shields.io/badge/Status-Complete-brightgreen)

## Features

- Animated Sharingan eye with spinning tomoe.
- Pulsing red iris for a dynamic effect.
- Flickering glint for added realism.

## Hardware Requirements

- **Arduino Nano, Uno, or ESP32**
- **WS2812B LED Matrix** (e.g., 9x22 or larger)
- **5V Power Supply**

## How It Works

The animation is created by drawing several layers:
1.  A pulsing red circle forms the iris.
2.  A black circle is drawn in the center for the pupil.
3.  Three black tomoe are drawn and rotated around the pupil.
4.  A random white glint is occasionally added for a flickering effect.

> [!IMPORTANT]
> This effect is designed for larger matrices (e.g., 9x22 or 198 LEDs). On smaller matrices, the details of the Sharingan eye may not be clearly visible.

## Configuration

```cpp
#define NUM_LEDS    220
#define ROWS 10
#define COLS 22
```
