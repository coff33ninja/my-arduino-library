# Arduino LED Matrix Spectrum Analyzer

This project transforms a WS2812B LED matrix into a multi-bar audio spectrum analyzer. It uses a microphone to listen to music and displays the frequency content as a series of bouncing, colored bars.

## How it Works

Instead of just reacting to volume, this code performs a **Fast Fourier Transform (FFT)** on the audio signal. This mathematical process breaks the sound down into its individual frequencies.

The frequencies are then grouped into several "bins" (in this case, 7). Each bin represents a slice of the audio spectrum, from low frequencies (bass) on the left to high frequencies (treble) on the right. The height of each of the 7 bars on the matrix corresponds to the intensity of the sound in that frequency range.

The visualization includes smooth decay effects and a rainbow color pattern across the bars for a dynamic "arcade disco" look.

## Components Needed

*   Arduino board (e.g., Uno, Nano)
*   Addressable LED Matrix (e.g., a 22x10 WS2812B matrix)
*   Microphone Sensor Module (e.g., MAX4466 or a generic sound sensor with an analog output)
*   Breadboard and jumper wires

## Wiring

1.  **Microphone Sensor:**
    *   `VCC` to `5V` on Arduino
    *   `GND` to `GND` on Arduino
    *   `OUT` (or `A0`) to `A0` on Arduino

2.  **LED Matrix:**
    *   `VCC` or `5V` to an external 5V power supply. **Do not power a full matrix directly from the Arduino's 5V pin!**
    *   `GND` to the external power supply's ground AND a `GND` pin on the Arduino.
    *   `Data In` (`DI`) to pin `6` on the Arduino (as defined in the code).

## Setup and Customization

1.  **Install Libraries:** Before uploading, you must install the following libraries through the Arduino IDE's Library Manager:
    *   `FastLED`
    *   `arduinoFFT`

2.  **Tune Sensitivity:** The most important part is tuning the visualizer to your specific microphone and audio source.
    *   Find this line in the code: `barValues[i] = map(peak, 0, 400, 0, ROWS);`
    *   The `400` is the sensitivity value.
    *   If the bars are too low or not reacting, **decrease** this number (e.g., to `200`).
    *   If the bars are always at maximum height, **increase** this number (e.g., to `600`).

3.  **Advanced Customization (in the code):**
    *   `NUM_BARS`: You can change the number of frequency bars displayed (e.g., from 7 to 5).
    *   `SAMPLES`: Controls the resolution of the FFT. Must be a power of 2. `64` is a good balance for performance.
    *   `fadeToBlackBy(40)`: Controls how quickly the bars fall. A higher value means a faster decay.