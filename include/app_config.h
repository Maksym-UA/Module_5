#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>

namespace AppConfig
{
constexpr char kLogTag[] = "PID";

constexpr float kPidKp = 0.8F;
constexpr float kPidKi = 0.05F;
constexpr float kPidKd = 0.02F;
constexpr float kPidDeadband = 3.0F;
constexpr float kTargetLightPercent = 70.0F;

constexpr float kOutputMin = 0.0F;
constexpr float kOutputMax = 127.0F;
constexpr float kInputFilterAlpha = 0.04F;
constexpr float kMaxBrightnessStepPerCycle = 1.0F;

constexpr bool kDefaultInvertSensorPercent = true;
constexpr uint8_t kPolarityProbeBrightness = 127;
constexpr TickType_t kPolaritySettleDelay = pdMS_TO_TICKS(180);
constexpr int kPolarityMinDeltaPercent = 2;
constexpr uint16_t kPolarityMinDeltaRaw = 10;

constexpr size_t kRawSamplesPerCycle = 11;

constexpr TickType_t kControlPeriod = pdMS_TO_TICKS(50);
constexpr int64_t kLogPeriodUs = 500000;
}