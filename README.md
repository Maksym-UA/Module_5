# Light Controller - ESP32-S3, ESP-IDF

## Overview

Firmware for ESP32-S3-DevKitC-1 that controls 5 LEDs with a closed-loop PID regulator using a photoresistor as feedback.

- Platform: ESP32-S3-DevKitC-1
- Framework: ESP-IDF via PlatformIO
- Sensor input: ADC oneshot (photoresistor on GPIO4)
- LED output: LEDC PWM on GPIO5..GPIO9

The controller targets a configured light level (`kTargetLightPercent`) and continuously adjusts LED brightness (0..127 duty units).

## Hardware

| Component | Value / Details |
|---|---|
| MCU | ESP32-S3-DevKitC-1 |
| Sensor | Photoresistor module on GPIO4 |
| LEDs | 5 LEDs on GPIO5, GPIO6, GPIO7, GPIO8, GPIO9 |
| PWM driver | LEDC low-speed mode, 8-bit resolution, 5 kHz |

## Wiring

Photoresistor:

- VCC -> 3.3V
- GND -> GND
- OUT -> GPIO4

LEDs:

- GPIO5 -> LED1
- GPIO6 -> LED2
- GPIO7 -> LED3
- GPIO8 -> LED4
- GPIO9 -> LED5

Use a current-limiting resistor for each LED.

## Software Requirements

- VS Code
- PlatformIO extension
- ESP-IDF toolchain (installed automatically by PlatformIO)

## Build, Upload, Monitor

Build:

```bash
pio run
```

Upload:

```bash
pio run -t upload
```

Serial monitor:

```bash
pio device monitor -b 115200
```

## Runtime Flow

1. `main.cpp` calls `application_init()`.
2. `Application::start()` initializes LED and ADC drivers.
3. A startup polarity probe measures sensor raw value with LEDs off and on.
4. Control loop runs every 50 ms:
- read ADC raw
- clamp accepted raw value (`kAcceptedRawMax`)
- convert raw to measured percent (probe span or fallback raw range)
- low-pass filter measurement
- compute PID output as brightness target
- apply per-cycle brightness slew limit
- write PWM duty to all LEDs

## Control Configuration

Current values from `include/app_config.h`:

- PID gains: Kp=0.8, Ki=0.05, Kd=0.02
- Deadband: 3.0
- Target light: 60.0%
- Output range: 0..127
- Input filter alpha: 0.04
- Max brightness step per cycle: 1.0
- Loop period: 50 ms
- Log period: 500 ms

Probe and conversion settings:

- Default invert fallback: true
- Probe brightness: 127
- Probe settle delay: 180 ms
- Probe min delta raw: 100
- Fallback control raw range: 18..55
- Accepted raw clamp max: 120

## Logging

Typical runtime log format:

```text
I (...) PID: raw=1120 effective=120 measured=57.2% filtered=54.8% target=60.0% brightness=74
```

Fields:

- `raw`: direct ADC sample
- `effective`: clamped value used by control path
- `measured`: mapped sensor percent before filtering
- `filtered`: low-pass filtered percent for PID input
- `target`: configured setpoint percent
- `brightness`: PWM duty command (0..127)

## Project Structure

```text
include/
  app_config.h
  application.h
  control/
    light_controller.h
    pid_controller.h
    sensor_probe.h
    sensor_reader.h
    signal_filter.h

lib/
  led/
    led.cpp
    led.h
  photoresistor/
    photoresistor.cpp
    photoresistor.h

src/
  application.cpp
  CMakeLists.txt
  main.cpp
  control/
    light_controller.cpp
    pid_controller.cpp
    sensor_probe.cpp
    sensor_reader.cpp
    signal_filter.cpp

CMakeLists.txt
platformio.ini
sdkconfig.esp32-s3-devkitc-1
```

## Contact

Feedback: max.savin3@gmail.com