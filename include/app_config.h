#pragma once

#include <Arduino.h>
#include <ESP32Encoder.h>
#include <PID_v1.h>


namespace AppConfig
{
// -------------------- Призначення пінів --------------------
#define PWM_OUT_PIN 5
#define ENCODER_A_PIN 15
#define ENCODER_B_PIN 16
#define POT_ADC_PIN 4

// -------------------- ШІМ --------------------
#define PWM_FREQ_HZ 20000
#define PWM_CHANNEL 0
#define PWM_RES_BITS 10
#define PWM_MAX_DUTY ((1 << PWM_RES_BITS) - 1)

// -------------------- Енкодер (PCNT, квадратурний x4) --------------------
#define ENCODER_CPR_X4 80

#define PCNT_HIGH_LIMIT 32767
#define PCNT_LOW_LIMIT -32768

// -------------------- ПІД-регулятор --------------------
#define PID_KP 0.1
#define PID_KI 0.1
#define PID_KD 0.1

// -------------------- Логіка керування --------------------
#define ANGLE_MIN_DEG 0.0
#define ANGLE_MAX_DEG 360.0

#define ADC_MIN 0.
#define ADC_MAX 4095
#define ADC_REARM_THRESHOLD 120

#define CONTROL_PERIOD_MS 20U
#define STATUS_PRINT_MS 200U

// -------------------- Тестування потенціометра --------------------
//POT_RAW_TEST_MODE 1 for raw ADC reading test mode, 0 for normal PID control mode
#define POT_RAW_TEST_MODE 0
#define POT_RAW_PRINT_MS 100U

// -------------------- Тестування енкодера --------------------
// ENCODER_TEST_MODE 1 for encoder readout test mode, 0 to disable
#define ENCODER_TEST_MODE 0
#define ENCODER_TEST_PRINT_MS 100U

}
