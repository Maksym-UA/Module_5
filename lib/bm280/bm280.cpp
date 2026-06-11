#include "bm280.h"

#include "i2c_bus.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "bm280";

namespace bme280_app {



uint16_t BME280::readU16LE(const uint8_t* data)
{
    return static_cast<uint16_t>(data[0]) |
           (static_cast<uint16_t>(data[1]) << 8);
}

int16_t BME280::readS16LE(const uint8_t* data)
{
    return static_cast<int16_t>(readU16LE(data));
}

// I2C 

bool BME280::readRegister(uint8_t reg, uint8_t* data, uint8_t len) const
{
    if (dev_handle_ == nullptr) {
        return false;
    }
    esp_err_t err = i2c_master_transmit_receive(dev_handle_, &reg, 1, data, len, /*timeout_ms=*/-1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C read reg 0x%02X failed: %s", reg, esp_err_to_name(err));
        return false;
    }
    return true;
}

bool BME280::writeRegister(uint8_t reg, uint8_t value) const
{
    if (dev_handle_ == nullptr) {
        return false;
    }
    uint8_t buf[2] = {reg, value};
    esp_err_t err = i2c_master_transmit(dev_handle_, buf, sizeof(buf), /*timeout_ms=*/-1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C write reg 0x%02X failed: %s", reg, esp_err_to_name(err));
        return false;
    }
    return true;
}

// Calibration

bool BME280::readCalibData()
{
    uint8_t calib1[26] = {};// calib data from 0x88 to 0xA1
    uint8_t calib2[7]  = {};// calib data from 0xE1 to 0xE7

    if (!readRegister(REG_CALIB00, calib1, sizeof(calib1))) {
        return false;
    }
    if (!readRegister(REG_CALIB26, calib2, sizeof(calib2))) {
        return false;
    }

    calib_.dig_T1 = readU16LE(&calib1[0]);
    calib_.dig_T2 = readS16LE(&calib1[2]);
    calib_.dig_T3 = readS16LE(&calib1[4]);

    calib_.dig_P1 = readU16LE(&calib1[6]);
    calib_.dig_P2 = readS16LE(&calib1[8]);
    calib_.dig_P3 = readS16LE(&calib1[10]);
    calib_.dig_P4 = readS16LE(&calib1[12]);
    calib_.dig_P5 = readS16LE(&calib1[14]);
    calib_.dig_P6 = readS16LE(&calib1[16]);
    calib_.dig_P7 = readS16LE(&calib1[18]);
    calib_.dig_P8 = readS16LE(&calib1[20]);
    calib_.dig_P9 = readS16LE(&calib1[22]);

    calib_.dig_H1 = calib1[25];
    calib_.dig_H2 = readS16LE(&calib2[0]);
    calib_.dig_H3 = calib2[2];
    calib_.dig_H4 = static_cast<int16_t>(
        (static_cast<int16_t>(calib2[3]) << 4) |
        (static_cast<int16_t>(calib2[4]) & 0x0F));
    calib_.dig_H5 = static_cast<int16_t>(
        (static_cast<int16_t>(calib2[5]) << 4) |
        (static_cast<int16_t>(calib2[4]) >> 4));

    // Sign-extend 12-bit values stored in int16_t.
    if ((calib_.dig_H4 & 0x0800) != 0) {
        calib_.dig_H4 |= static_cast<int16_t>(0xF000);
    }
    if ((calib_.dig_H5 & 0x0800) != 0) {
        calib_.dig_H5 |= static_cast<int16_t>(0xF000);
    }
    calib_.dig_H6 = static_cast<int8_t>(calib2[6]);

    return true;
}


// Bosch compensation formulas (integer, from datasheet section 8.2)

int32_t BME280::compensateTemp(int32_t adc_T) const
{
    const int32_t var1 =
        ((((adc_T >> 3) - (static_cast<int32_t>(calib_.dig_T1) << 1))) *
         static_cast<int32_t>(calib_.dig_T2)) >> 11;

    const int32_t var2 =
        (((((adc_T >> 4) - static_cast<int32_t>(calib_.dig_T1)) *
           ((adc_T >> 4) - static_cast<int32_t>(calib_.dig_T1))) >> 12) *
         static_cast<int32_t>(calib_.dig_T3)) >> 14;

    tFine_ = var1 + var2;
    return (tFine_ * 5 + 128) >> 8;
}

uint32_t BME280::compensatePress(int32_t adc_P) const
{
    int64_t var1 = static_cast<int64_t>(tFine_) - 128000;
    int64_t var2 = var1 * var1 * static_cast<int64_t>(calib_.dig_P6);
    var2 += (var1 * static_cast<int64_t>(calib_.dig_P5)) << 17;
    var2 += static_cast<int64_t>(calib_.dig_P4) << 35;
    var1  = ((var1 * var1 * static_cast<int64_t>(calib_.dig_P3)) >> 8) +
            ((var1 * static_cast<int64_t>(calib_.dig_P2)) << 12);
    var1  = (((static_cast<int64_t>(1) << 47) + var1) *
              static_cast<int64_t>(calib_.dig_P1)) >> 33;
    if (var1 == 0) {
        return 0;
    }
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (static_cast<int64_t>(calib_.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (static_cast<int64_t>(calib_.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (static_cast<int64_t>(calib_.dig_P7) << 4);
    return static_cast<uint32_t>(p);
}

uint32_t BME280::compensateHumid(int32_t adc_H) const
{
    int32_t v = tFine_ - 76800;
    v = (((((adc_H << 14) -
            (static_cast<int32_t>(calib_.dig_H4) << 20) -
            (static_cast<int32_t>(calib_.dig_H5) * v)) + 16384) >> 15) *
         (((((((v * static_cast<int32_t>(calib_.dig_H6)) >> 10) *
              (((v * static_cast<int32_t>(calib_.dig_H3)) >> 11) + 32768)) >> 10) +
            2097152) *
           static_cast<int32_t>(calib_.dig_H2) + 8192) >> 14));
    v -= (((((v >> 15) * (v >> 15)) >> 7) *
           static_cast<int32_t>(calib_.dig_H1)) >> 4);
    if (v < 0)         { v = 0;         }
    if (v > 419430400) { v = 419430400; }
    return static_cast<uint32_t>(v >> 12);
}

// Public API

bool BME280::begin(int sdaPin, int sclPin, uint8_t address, uint32_t clockHz)
{
    initialized_ = false;

    esp_err_t err = app_i2c::acquire_bus(sdaPin, sclPin, &bus_handle_);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to acquire shared I2C bus: %s", esp_err_to_name(err));
        return false;
    }

    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length  = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address   = address;
    dev_cfg.scl_speed_hz     = clockHz;

    err = i2c_master_bus_add_device(bus_handle_, &dev_cfg, &dev_handle_);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_master_bus_add_device failed: %s", esp_err_to_name(err));
        return false;
    }

    uint8_t chip_id = 0;
    if (!readRegister(REG_CHIP_ID, &chip_id, 1)) {
        ESP_LOGE(TAG, "Failed to read chip ID");
        return false;
    }
    if (chip_id != 0x60) {
        ESP_LOGE(TAG, "Unexpected chip ID: 0x%02X (expected 0x60)", chip_id);
        return false;
    }

    if (!readCalibData())               { return false; }
    if (!writeRegister(REG_CTRL_HUM,  0x01)) { return false; }
    if (!writeRegister(REG_CTRL_MEAS, 0x27)) { return false; }
    if (!writeRegister(REG_CONFIG,    0xA0)) { return false; }

    // Allow first measurement to complete.
    vTaskDelay(pdMS_TO_TICKS(50));
    initialized_ = true;
    ESP_LOGI(TAG, "BME280 ready (chip ID 0x%02X)", chip_id);
    return true;
}

bool BME280::readData(BME280Data& out)
{
    if (!initialized_) {
        return false;
    }

    uint8_t buffer[8] = {};
    if (!readRegister(REG_DATA_START, buffer, sizeof(buffer))) {
        return false;
    }

    const int32_t adc_P =
        (static_cast<int32_t>(buffer[0]) << 12) |
        (static_cast<int32_t>(buffer[1]) <<  4) |
        (static_cast<int32_t>(buffer[2]) >>  4);

    const int32_t adc_T =
        (static_cast<int32_t>(buffer[3]) << 12) |
        (static_cast<int32_t>(buffer[4]) <<  4) |
        (static_cast<int32_t>(buffer[5]) >>  4);

    const int32_t adc_H =
        (static_cast<int32_t>(buffer[6]) << 8) |
         static_cast<int32_t>(buffer[7]);

    // Temperature must be compensated first to set tFine_.
    const int32_t  tempX100    = compensateTemp(adc_T);
    const uint32_t pressQ24_8  = compensatePress(adc_P);
    const uint32_t humQ22_10   = compensateHumid(adc_H);

    out.temperatureC    = static_cast<float>(tempX100)   / 100.0F;
    out.pressureHpa     = static_cast<float>(pressQ24_8) / 25600.0F;
    out.humidityPercent = static_cast<float>(humQ22_10)  / 1024.0F;

    return true;
}

} // namespace bme280_app
