# Button Debounce with RC Filter — ESP32-S3, ESP-IDF

## Project description

Demonstrates hardware + software button debouncing using an external tactile push-button and a passive RC low-pass filter (10 kΩ + 100 nF, τ = 1 ms). The RC network suppresses mechanical bounce spikes before they reach the GPIO pin. A 5 ms software guard time in the event handler eliminates any remaining glitches. Confirmed press and release events are logged over UART and reflected on an LED.



## Hardware

| Component | Value / Details |
|---|---|
| MCU | ESP32-S3-DevKitC-1 |
| Button | External push-button (active-high) |
| Resistor | 10 kΩ |
| Capacitor | 100 nF |
| LED | Red + 220 Ω series resistor |

## Wiring

```
3.3V ──┬── R(10kΩ) ──┬── GPIO_NUM_8 (button input, no internal pull)
       │              │
  EXT BTN            C(100nF)
       │              │
      GND ───────────GND

GPIO_NUM_9 ── LED ── 220Ω ── GND
```

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

## Project structure

```
src/
  main.cpp          - Entry point: app_main() calls application_init()
  application.cpp   - ISR, FreeRTOS task, GPIO config, debounce logic
  application.h     - Public interface: declares application_init()
platformio.ini      - Board and build settings
sdkconfig.esp32-s3-devkitc-1  - ESP-IDF sdkconfig
```

## Contact

Feedback: max.savin3@gmail.com

