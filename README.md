# Motor PID Controller - ESP32-S3, ESP-IDF

## Overview

The motor is PWM-controlled via LEDS.
Encoder readings via через PCNT (quadrature x4)
Process Potentiometer position
Motor shaft position is controlled by PID

## Hardware

| Component | Value / Details |
|---|---|
| MCU | ESP32-S3-DevKitC-1 |
| Motor | Direct current motor |
| Encoder | KY-040 module with mechanical EC11 encoder |
| PWM control | NPN-transistor for PWM control of the motor |
| | Potentiometer |


## Wiring



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

when turning the potetiometer cloclwise (increasing voltage at ADC input) the motor is supposed to alter its shaft position according to the potemtiomer position. 
When altering voltage at ADC input within 0–3.3 V the motor is supposed to rotate within 0° до 360° range.
Motor position control is to be implemented via encoder. 
The PID regulator ueses these coefficients:
PID_KP
PID_KI
PID_KD
При досягненні максимального положення потенціометра або при повороті потенціометра проти годинникової стрілки двигун повинен зупинятися.
Для поновлення регулювання необхідно повернути потенціометр у початкове положення (мінімальна вихідна напруга).

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
```

## Contact

Feedback: max.savin3@gmail.com