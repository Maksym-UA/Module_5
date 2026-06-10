#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <cstring>

#include "DS1307clock.hpp"
#include "BME280.hpp"

namespace oled_app {

    class SSD1306Display {
    public:
        // Constructor initializes the display object with the appropriate settings for
        // the SSD1306 128x64 OLED display.
        SSD1306Display() : u8g2_(U8G2_R0, U8X8_PIN_NONE) {}

        void begin() {
            u8g2_.begin();
            u8g2_.enableUTF8Print();
            // The U8g2 library expects the 7-bit I2C address to be left-shifted by 1
            u8g2_.setI2CAddress(OLED_ADDR << 1);
            u8g2_.setBusClock(100000);
        }

        void showStartupMessage(const char* message) {
            u8g2_.clearBuffer();
            u8g2_.setFont(u8g2_font_6x12_tr);
            u8g2_.drawStr(2, 12, message);
            u8g2_.sendBuffer();
        }

        void showError(const char* message) {
            u8g2_.clearBuffer();
            u8g2_.setFont(u8g2_font_6x12_tr);
            u8g2_.drawStr(2, 20, message);
            u8g2_.sendBuffer();
        }

        // Pass bmeData to show sensor values in the footer, or nullptr to hide footer values.
        void showRtcFallbackTime(
            const char* dateTimeText,
            const bme280_app::BME280Data* bmeData = nullptr) {
            drawRtcFallbackTime(dateTimeText, bmeData);
        }

        // Pass bmeData to show sensor values in the footer, or nullptr to hide footer values.
        void showDateTime(
            const clock_app::DateTime& dateTime,
            const bme280_app::BME280Data* bmeData = nullptr) {
            drawDateTime(dateTime, bmeData);
        }

    private:
        static constexpr uint8_t OLED_ADDR = 0x3C;
        U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2_;

        static void extractTimeText(const char* dateTimeText, char* out, size_t outLen) {
            if (out == nullptr || outLen == 0) {
                return;
            }

            snprintf(out, outLen, "--:--:--");
            if (dateTimeText != nullptr && strlen(dateTimeText) >= 19) {
                snprintf(out, outLen, "%s", dateTimeText + 11);
            }
        }

        static void formatBmeText(const bme280_app::BME280Data& bmeData, char* out, size_t outLen) {
            if (out == nullptr || outLen == 0) {
                return;
            }

            snprintf(
                out,
                outLen,
                "T:%.1fC H:%.0f%% P:%.0fhPa",
                bmeData.temperatureC,
                bmeData.humidityPercent,
                bmeData.pressureHpa);
        }

        void drawRtcFallbackTime(
            const char* dateTimeText,
            const bme280_app::BME280Data* bmeData) {

            char timeText[9];
            extractTimeText(dateTimeText, timeText, sizeof(timeText));

            char footerText[24];
            const char* footer = "System time";
            if (bmeData != nullptr) {
                formatBmeText(*bmeData, footerText, sizeof(footerText));
                footer = footerText;
            }

            u8g2_.clearBuffer();
            u8g2_.setFont(u8g2_font_6x12_tr);
            u8g2_.drawStr(2, 12, "RTC unavailable");
            u8g2_.setFont(u8g2_font_logisoso20_tn);
            u8g2_.drawStr(2, 42, timeText);
            u8g2_.setFont(u8g2_font_6x12_tr);
            u8g2_.drawStr(2, 56, footer);
            u8g2_.sendBuffer();
        }

        void drawDateTime(
            const clock_app::DateTime& dateTime,
            const bme280_app::BME280Data* bmeData) {

            char timeText[9];
            char dateText[20];

            snprintf(
                timeText,
                sizeof(timeText),
                "%02u:%02u:%02u",
                dateTime.hour,
                dateTime.minute,
                dateTime.second);

            snprintf(
                dateText,
                sizeof(dateText),
                "%s %02u.%02u.%04u",
                clock_app::DS1307clock::dayToShortName(dateTime.dayOfWeek),
                dateTime.dayOfMonth,
                dateTime.month,
                dateTime.year);

            u8g2_.clearBuffer();
            u8g2_.setFont(u8g2_font_6x12_tr);
            u8g2_.drawStr(2, 12, dateText);

            u8g2_.setFont(u8g2_font_logisoso20_tn);
            const uint8_t timeY = (bmeData == nullptr) ? 50 : 42;
            u8g2_.drawStr(2, timeY, timeText);

            if (bmeData != nullptr) {
                char bmeText[24];
                formatBmeText(*bmeData, bmeText, sizeof(bmeText));
                u8g2_.setFont(u8g2_font_6x12_tr);
                u8g2_.drawStr(2, 56, bmeText);
            }

            u8g2_.sendBuffer();
        }
    };
}