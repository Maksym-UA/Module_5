# Button Debounce with RC Filter — ESP32-S3, ESP-IDF

## Project description

Demonstrates button debouncing on ESP32-S3 using an external 3-pin button module (VCC, OUT, GND) plus an RC filter (10 kOhm and 100 nF). The firmware uses a fast polling task with software lockout debounce. Only valid **press** events are counted; each press toggles the LED state.



## Hardware

| Component | Value / Details |
|---|---|
| MCU | ESP32-S3-DevKitC-1 |
| Button | External 3-pin module (VCC, OUT, GND) |
| Resistor | 10 kOhm |
| Capacitor | 100 nF |
| LED | Red + 220 Ω series resistor |

## Wiring

```
Button module VCC -> ESP32 3V3
Button module GND -> ESP32 GND
Button module OUT -> 10 kOhm -> GPIO_NUM_8
GPIO_NUM_8 -> 100 nF -> GND

GPIO_NUM_9 ── LED ── 220Ω ── GND
```

3-pin button module mapping:

- VCC -> 3.3V
- OUT -> GPIO input pin
- GND -> Ground

Current firmware uses active-high logic: released = 0, pressed = 1.
If your module is active-low, set `BUTTON_ACTIVE_LEVEL` to `0` in `src/application.cpp`.

> Adjust `BUTTON_GPIO` and `LED_GPIO` in `src/application.cpp` to match your board.

## Software requirements

- VS Code
- PlatformIO extension
- ESP-IDF toolchain (installed automatically by PlatformIO)

## Build and run

Build firmware:

```bash
pio run
```

Upload firmware:

```bash
pio run -t upload
```

Open serial monitor (115200 baud):

```bash
pio device monitor -b 115200
```

## Configuration

- Framework: `espidf`
- Monitor speed: `115200`
- Flash mode/size: `qio`, `16 MB`

## Debounce behavior (current)

- Sampling period: `SAMPLE_PERIOD_MS = 1`
- Click lockout: `CLICK_LOCKOUT_MS = 30`
- Release re-arm stability: `RELEASE_STABLE_SAMPLES = 3`

How it works:

1. Detect edge quickly via polling.
2. Count click only when button enters active level and press is not latched.
3. Start lockout timer to ignore bounce spikes.
4. Re-arm next click only after release level is stable for several samples.

This gives responsive clicks while preventing duplicate counts.

## Project structure

```
src/
  main.cpp          - Entry point: app_main() calls application_init()
  application.cpp   - FreeRTOS polling task, GPIO config, debounce and LED toggle logic
  application.h     - Public interface: declares application_init()
platformio.ini      - Board and build settings
sdkconfig.esp32-s3-devkitc-1  - ESP-IDF sdkconfig
```

## Contact

Feedback: max.savin3@gmail.com

