#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>

namespace AppConfig
{
constexpr char kLogTag[] = "PID";

constexpr float kPidKp = 1.0F;
constexpr float kPidKi = 0.0F;
constexpr float kPidKd = 0.02F;
constexpr float kPidDeadband = 5.0F;
constexpr float kTargetLightPercent = 80.0F;

constexpr float kOutputMin = 0.0F;
constexpr float kOutputMax = 120.0F;
constexpr float kInputFilterAlpha = 0.15F;
constexpr float kMaxBrightnessStepPerCycle = 1.0F;

constexpr bool kDefaultInvertSensorPercent = true;
constexpr uint8_t kPolarityProbeBrightness = 127;
constexpr TickType_t kPolaritySettleDelay = pdMS_TO_TICKS(180);
constexpr int kPolarityMinDeltaPercent = 2;
constexpr uint16_t kPolarityMinDeltaRaw = 10;

constexpr size_t kRawSamplesPerCycle = 5;
constexpr uint8_t kZeroRawDebounceCycles = 12;


constexpr float kThresholdRawLowRatio = 0.08F;
constexpr float kThresholdRawHighRatio = 0.18F;
constexpr float kThresholdStepUp = 0.8F;
constexpr float kThresholdStepDown = 1.0F;
constexpr uint8_t kThresholdDebounceCycles = 3;
constexpr uint8_t kThresholdAdjustCooldownCycles = 8;
constexpr bool kEnableThresholdController = false;

constexpr TickType_t kControlPeriod = pdMS_TO_TICKS(50);
constexpr int64_t kLogPeriodUs = 500000;
}