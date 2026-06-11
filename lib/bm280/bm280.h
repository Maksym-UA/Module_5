#pragma once

#include <stdint.h>
#include "driver/i2c_master.h"

namespace bme280_app {

struct BME280Data {
    float temperatureC = 0.0F;
    float humidityPercent = 0.0F;
    float pressureHpa = 0.0F;
};

class BME280 {
public:
    static constexpr uint8_t  kDefaultAddress    = 0x76;
    static constexpr uint32_t kDefaultI2cClockHz = 100000;

    // Initialise the sensor.  sdaPin / sclPin are GPIO numbers.
    bool begin(int sdaPin, int sclPin,
               uint8_t address   = kDefaultAddress,
               uint32_t clockHz  = kDefaultI2cClockHz);

    // Read temperature (°C), humidity (%), and pressure (hPa) from the sensor.
    bool readData(BME280Data& out);

private:
    static constexpr uint8_t REG_CHIP_ID    = 0xD0;
    static constexpr uint8_t REG_CTRL_HUM   = 0xF2;
    static constexpr uint8_t REG_CTRL_MEAS  = 0xF4;
    static constexpr uint8_t REG_CONFIG     = 0xF5;
    static constexpr uint8_t REG_DATA_START = 0xF7;
    static constexpr uint8_t REG_CALIB00    = 0x88;
    static constexpr uint8_t REG_CALIB26    = 0xE1;

    struct CalibData {
        uint16_t dig_T1 = 0;
        int16_t  dig_T2 = 0;
        int16_t  dig_T3 = 0;
        uint16_t dig_P1 = 0;
        int16_t  dig_P2 = 0;
        int16_t  dig_P3 = 0;
        int16_t  dig_P4 = 0;
        int16_t  dig_P5 = 0;
        int16_t  dig_P6 = 0;
        int16_t  dig_P7 = 0;
        int16_t  dig_P8 = 0;
        int16_t  dig_P9 = 0;
        uint8_t  dig_H1 = 0;
        int16_t  dig_H2 = 0;
        uint8_t  dig_H3 = 0;
        int16_t  dig_H4 = 0;
        int16_t  dig_H5 = 0;
        int8_t   dig_H6 = 0;
    };

    // I2C handles and calibration data
    i2c_master_bus_handle_t bus_handle_  = nullptr;
    i2c_master_dev_handle_t dev_handle_  = nullptr;
    bool initialized_ = false;
    CalibData calib_  = {};
    mutable int32_t tFine_ = 0;

    bool readRegister(uint8_t reg, uint8_t* data, uint8_t len) const; // Read multiple bytes starting from reg
    bool writeRegister(uint8_t reg, uint8_t value) const;
    bool readCalibData();

    static uint16_t readU16LE(const uint8_t* data);// Read unsigned 16-bit little-endian from byte array
    static int16_t  readS16LE(const uint8_t* data);// Read unsigned/signed 16-bit little-endian from byte array

    int32_t  compensateTemp(int32_t adc_T)  const;
    uint32_t compensatePress(int32_t adc_P) const;
    uint32_t compensateHumid(int32_t adc_H) const;
};

} // namespace bme280_app
