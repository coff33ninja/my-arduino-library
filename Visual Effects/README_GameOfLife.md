# Conway's Game of Life for LED Matrix

A simulation of John Conway's Game of Life for WS2812B LED matrices.

![Game of Life Demo](https://img.shields.io/badge/Status-In_Development-orange)

## Features

- Simulates Conway's Game of Life on a 2D LED grid.
- Starts with a random initial pattern.
- Wraps around the edges for a continuous world.
- Configurable simulation speed.

## Hardware Requirements

- **Arduino Nano, Uno, or ESP32**
- **WS2812B LED Matrix** (e.g., 10x22)
- **5V Power Supply**

## How It Works

The sketch uses two 2D arrays to hold the state of each cell (alive or dead) for the current and next generation. In each cycle, it calculates the state of each cell in the next generation based on the classic Game of Life rules:

1.  A live cell with fewer than two live neighbours dies.
2.  A live cell with two or three live neighbours lives on.
3.  A live cell with more than three live neighbours dies.
4.  A dead cell with exactly three live neighbours becomes a live cell.

## Configuration

```cpp
#define MATRIX_WIDTH 22
#define MATRIX_HEIGHT 10
#define SIMULATION_SPEED 100 // ms delay between generations
```

## Optional Features

### Speed Control

To control the simulation speed dynamically, you can use a potentiometer. 

1.  Connect a potentiometer to pin `A1`.
2.  In `GameOfLife.ino`, uncomment the following line:

```cpp
// #define USE_POT_SPEED_CONTROL
```

This will override the `SIMULATION_SPEED` constant and use the potentiometer's value to control the delay between generations.
