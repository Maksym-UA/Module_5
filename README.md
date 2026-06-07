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
| PWM control | NPN-транзистор для PWM-керування двигуном |
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

При повороті потенціометра за годинниковою стрілкою (зі збільшенням напруги на вході АЦП) двигун повинен змінювати своє положення відповідно до положення потенціометра.
При зміні напруги на вході АЦП у діапазоні 0–3.3 В двигун повинен повертатися в діапазоні від 0° до 360°.
Контроль положення двигуна необхідно реалізувати за допомогою енкодера.
Необхідно реалізувати PID-регулятор та підібрати коефіцієнти:
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