# Motor PID Controller - ESP32-S3 (PlatformIO + Arduino)

## Overview

This project controls DC motor position using:

- PWM output (LEDC)
- Quadrature encoder feedback (ESP32 PCNT via `ESP32Encoder`)
- Potentiometer input (ADC)
- PID position control (`PID_v1`)

The firmware also includes dedicated test modes for:

- Raw potentiometer ADC readout
- Encoder count and angle readout

## Hardware

| Component | Value / Details |
|---|---|
| MCU | ESP32-S3-DevKitC-1 |
| Motor | DC motor |
| Encoder | KY-040 (EC11 mechanical encoder) |
| Potentiometer | Analog position input (0-3.3 V to ADC) |
| PWM stage | NPN transistor motor driver stage |
| Discrete transistor stage parts | ST2N2222A NPN transistor + 1 kOhm base resistor |
| Flyback protection | 1N4007 diode across motor terminals |

Note: connect motor via a proper driver/transistor stage, not directly to ESP32 pin.

## Software Requirements

- VS Code
- PlatformIO extension
- PlatformIO Core with Arduino-ESP32 toolchain

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

## Runtime Modes

Mode is selected in `include/app_config.h`.

### 1) PID Control Mode (default)

Set:

```c
#define ENCODER_TEST_MODE 0
#define POT_RAW_TEST_MODE 0
```

Behavior:

- Potentiometer ADC is converted to setpoint angle in range 0..360 deg
- Encoder count is converted to measured angle
- PID computes PWM duty to move shaft toward setpoint
- Control loop timing is defined by `CONTROL_PERIOD_MS`

### 2) Potentiometer Raw Test Mode

Set:

```c
#define ENCODER_TEST_MODE 0
#define POT_RAW_TEST_MODE 1
```

Serial output:

- `POT RAW ADC=<value>` every `POT_RAW_PRINT_MS`

### 3) Encoder Test Mode

Set:

```c
#define ENCODER_TEST_MODE 1
```

(`ENCODER_TEST_MODE` has higher priority than `POT_RAW_TEST_MODE`.)

Serial output:

- `ENC RAW CNT=<count> ANG_CONT=<deg> ANG_WRAP=<deg>` every `ENCODER_TEST_PRINT_MS`
- `ANG_CONT` is continuous accumulated angle
- `ANG_WRAP` is wrapped one-turn angle in range 0..360

## Important Configuration

Main parameters are in `include/app_config.h`:

- Pins: `PWM_OUT_PIN`, `ENCODER_A_PIN`, `ENCODER_B_PIN`, `POT_ADC_PIN`
- PWM: `PWM_FREQ_HZ`, `PWM_RES_BITS`, `PWM_CHANNEL`
- Encoder scaling: `ENCODER_CPR_X4`
- PID gains: `PID_KP`, `PID_KI`, `PID_KD`
- Limits/timing: `ADC_*`, `ANGLE_*`, `CONTROL_PERIOD_MS`, `STATUS_PRINT_MS`

## Project Structure

```text
include/
  app_config.h
  application.h

src/
  application.cpp
  CMakeLists.txt
  main.cpp

CMakeLists.txt
platformio.ini
sdkconfig.esp32-s3-devkitc-1
README.md
```

## Contact

Feedback: max.savin3@gmail.com