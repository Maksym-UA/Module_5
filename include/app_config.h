#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>

namespace AppConfig
{
constexpr char kLogTag[] = "PID";

constexpr float kPidKp = 0.8F;
constexpr float kPidKi = 0.08F;
constexpr float kPidKd = 0.02F;
constexpr float kPidDeadband = 3.0F;
constexpr float kTargetLightPercent = 60.0F; // Desired light level in percent (0-100).

constexpr float kOutputMin = 0.0F;
constexpr float kOutputMax = 127.0F;
constexpr float kInputFilterAlpha = 0.02F;
constexpr float kMaxBrightnessStepPerCycle = 1.0F; // Maximum change in brightness per control cycle to prevent abrupt changes.

constexpr bool kDefaultInvertSensorPercent = true; // Default setting for whether to invert the sensor percentage (true means higher raw values correspond to lower percentages).
constexpr uint8_t kPolarityProbeBrightness = 127; // Brightness level used during sensor inversion probing.
constexpr TickType_t kPolaritySettleDelay = pdMS_TO_TICKS(180); // Delay to allow sensor readings to stabilize after changing LED brightness during inversion detection.
constexpr uint16_t kPolarityMinDeltaRaw = 100; // Minimum change in raw sensor value to detect inversion.

constexpr uint16_t kControlRawMin = 18; // Minimum raw value corresponding to 0% light level for control purposes (calibrated based on sensor characteristics).
constexpr uint16_t kControlRawMax = 80; // Maximum raw value corresponding to 100% light level for control purposes (calibrated based on sensor characteristics).
constexpr uint16_t kAcceptedRawMax = 120; // Maximum raw value accepted from the sensor (beyond this value, readings are considered invalid).

constexpr TickType_t kControlPeriod = pdMS_TO_TICKS(50);
constexpr int64_t kLogPeriodUs = 500000;
}
