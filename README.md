# Light Controller — ESP32-S3, ESP-IDF

## Project Description

This firmware controls 5 LEDs using PWM based on a photoresistor reading.

- Platform: ESP32-S3-DevKitC-1
- Framework: ESP-IDF (via PlatformIO)
- Sensor input: ADC oneshot (photoresistor)
- LED output: LEDC PWM on 5 channels

The control logic is optimized for a near-threshold sensor response, where readings can jump between dark and bright states instead of changing smoothly.

## Hardware

| Component | Value / Details |
|---|---|
| MCU | ESP32-S3-DevKitC-1 |
| Sensor | Photoresistor module on GPIO4 |
| LEDs | 5 LEDs on GPIO 5, 6, 7, 8, 9 |
| PWM driver | LEDC low-speed mode |

## Wiring

Photoresistor:

- VCC -> 3.3V
- GND -> GND
- OUT -> GPIO4

LEDs:

- GPIO5 -> LED 1
- GPIO6 -> LED 2
- GPIO7 -> LED 3
- GPIO8 -> LED 4
- GPIO9 -> LED 5

Use proper current-limiting resistors for each LED.

## Software Requirements

- VS Code
- PlatformIO extension
- ESP-IDF toolchain (installed automatically by PlatformIO)

## Build And Run

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

## Current Control Strategy

The firmware contains two control paths:

1. PID path (fallback):
- Uses filtered sensor percentage and standard PID terms.

2. Threshold-hysteresis path (active when probe span is valid):
- Runs an automatic startup probe with LEDs off/on.
- Captures `off_raw` and `on_raw`.
- Builds low/high thresholds from that span.
- Increases brightness when raw is persistently below low threshold.
- Decreases brightness when raw is persistently above high threshold.
- Holds brightness inside the hysteresis band.
- Adds debounce and cooldown to reduce visible flicker.
- Applies bounded zero-reading debounce to reject short ADC zero glitches.

This approach is more stable than pure PID when the sensor behaves like a switch near an optical threshold.

## Important Notes

- The logged target percentage is a sensor target, not direct LED duty.
- Brightness values are PWM duty on a 0..127 scale.
- A value such as brightness 90 does not mean 90%; it means 90/127 duty.

## Project Structure

```text
include/
  application.h

src/
  main.cpp
  application.cpp

lib/
  led/
    led.h
    led.cpp
  photoresistor/
    photoresistor.h
    photoresistor.cpp

platformio.ini
sdkconfig.esp32-s3-devkitc-1
CMakeLists.txt
```

## Logging

Typical runtime log format:

```text
I (...) PID: raw=271 sensor=6% control=52.9% target=80.0% brightness=97
```

Fields:

- raw: ADC raw value
- sensor: direct percent from ADC raw (0..100)
- control: filtered/normalized control value used by controller
- target: desired control setpoint
- brightness: PWM duty command (0..127)

## Contact

Feedback: max.savin3@gmail.com