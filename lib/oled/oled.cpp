#include "oled.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "i2c_bus.h"

namespace oled_app {

static const char *TAG = "oled";
static constexpr uint8_t kI2cAddress = 0x3C;
static constexpr uint8_t kDisplayWidth = 128;
static constexpr uint8_t kDisplayHeight = 64;
static constexpr uint8_t kPageCount = kDisplayHeight / 8;

static i2c_master_dev_handle_t s_dev_handle = nullptr;
static bool s_ready = false;
static uint8_t s_framebuffer[kDisplayWidth * kPageCount] = {};

static const uint8_t kGlyphSpace[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t kGlyphPercent[5] = {0x63, 0x13, 0x08, 0x64, 0x63};
static const uint8_t kGlyphHyphen[5] = {0x08, 0x08, 0x08, 0x08, 0x08};
static const uint8_t kGlyphPeriod[5] = {0x00, 0x60, 0x60, 0x00, 0x00};
static const uint8_t kGlyphColon[5] = {0x00, 0x36, 0x36, 0x00, 0x00};

static const uint8_t kGlyph0[5] = {0x3E, 0x51, 0x49, 0x45, 0x3E};
static const uint8_t kGlyph1[5] = {0x00, 0x42, 0x7F, 0x40, 0x00};
static const uint8_t kGlyph2[5] = {0x42, 0x61, 0x51, 0x49, 0x46};
static const uint8_t kGlyph3[5] = {0x21, 0x41, 0x45, 0x4B, 0x31};
static const uint8_t kGlyph4[5] = {0x18, 0x14, 0x12, 0x7F, 0x10};
static const uint8_t kGlyph5[5] = {0x27, 0x45, 0x45, 0x45, 0x39};
static const uint8_t kGlyph6[5] = {0x3C, 0x4A, 0x49, 0x49, 0x30};
static const uint8_t kGlyph7[5] = {0x01, 0x71, 0x09, 0x05, 0x03};
static const uint8_t kGlyph8[5] = {0x36, 0x49, 0x49, 0x49, 0x36};
static const uint8_t kGlyph9[5] = {0x06, 0x49, 0x49, 0x29, 0x1E};

static const uint8_t kGlyphA[5] = {0x7E, 0x11, 0x11, 0x11, 0x7E};
static const uint8_t kGlyphB[5] = {0x7F, 0x49, 0x49, 0x49, 0x36};
static const uint8_t kGlyphC[5] = {0x3E, 0x41, 0x41, 0x41, 0x22};
static const uint8_t kGlyphD[5] = {0x7F, 0x41, 0x41, 0x22, 0x1C};
static const uint8_t kGlyphE[5] = {0x7F, 0x49, 0x49, 0x49, 0x41};
static const uint8_t kGlyphF[5] = {0x7F, 0x09, 0x09, 0x09, 0x01};
static const uint8_t kGlyphG[5] = {0x3E, 0x41, 0x49, 0x49, 0x7A};
static const uint8_t kGlyphH[5] = {0x7F, 0x08, 0x08, 0x08, 0x7F};
static const uint8_t kGlyphI[5] = {0x00, 0x41, 0x7F, 0x41, 0x00};
static const uint8_t kGlyphJ[5] = {0x20, 0x40, 0x41, 0x3F, 0x01};
static const uint8_t kGlyphK[5] = {0x7F, 0x08, 0x14, 0x22, 0x41};
static const uint8_t kGlyphL[5] = {0x7F, 0x40, 0x40, 0x40, 0x40};
static const uint8_t kGlyphM[5] = {0x7F, 0x02, 0x0C, 0x02, 0x7F};
static const uint8_t kGlyphN[5] = {0x7F, 0x04, 0x08, 0x10, 0x7F};
static const uint8_t kGlyphO[5] = {0x3E, 0x41, 0x41, 0x41, 0x3E};
static const uint8_t kGlyphP[5] = {0x7F, 0x09, 0x09, 0x09, 0x06};
static const uint8_t kGlyphQ[5] = {0x3E, 0x41, 0x51, 0x21, 0x5E};
static const uint8_t kGlyphR[5] = {0x7F, 0x09, 0x19, 0x29, 0x46};
static const uint8_t kGlyphS[5] = {0x46, 0x49, 0x49, 0x49, 0x31};
static const uint8_t kGlyphT[5] = {0x01, 0x01, 0x7F, 0x01, 0x01};
static const uint8_t kGlyphU[5] = {0x3F, 0x40, 0x40, 0x40, 0x3F};
static const uint8_t kGlyphV[5] = {0x1F, 0x20, 0x40, 0x20, 0x1F};
static const uint8_t kGlyphW[5] = {0x3F, 0x40, 0x38, 0x40, 0x3F};
static const uint8_t kGlyphX[5] = {0x63, 0x14, 0x08, 0x14, 0x63};
static const uint8_t kGlyphY[5] = {0x07, 0x08, 0x70, 0x08, 0x07};
static const uint8_t kGlyphZ[5] = {0x61, 0x51, 0x49, 0x45, 0x43};

static esp_err_t ensure_ready()
{
    if (!s_ready) {
        ESP_LOGE(TAG, "OLED not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    return ESP_OK;
}

static int32_t to_scaled_100(float value)
{
    const float scaled = value * 100.0F;
    return static_cast<int32_t>(scaled >= 0.0F ? scaled + 0.5F : scaled - 0.5F);
}

static void format_scaled_100(char *buffer, size_t buffer_size, float value)
{
    const int32_t scaled = to_scaled_100(value);
    const int32_t whole = scaled / 100;
    const int32_t fraction = scaled >= 0 ? (scaled % 100) : -(scaled % 100);
    snprintf(buffer, buffer_size, "%ld.%02ld", static_cast<long>(whole), static_cast<long>(fraction));
}

static const uint8_t *lookup_glyph(char c)
{
    if (c >= 'a' && c <= 'z') {
        c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    }

    switch (c) {
        case ' ': return kGlyphSpace;
        case '%': return kGlyphPercent;
        case '-': return kGlyphHyphen;
        case '.': return kGlyphPeriod;
        case ':': return kGlyphColon;
        case '0': return kGlyph0;
        case '1': return kGlyph1;
        case '2': return kGlyph2;
        case '3': return kGlyph3;
        case '4': return kGlyph4;
        case '5': return kGlyph5;
        case '6': return kGlyph6;
        case '7': return kGlyph7;
        case '8': return kGlyph8;
        case '9': return kGlyph9;
        case 'A': return kGlyphA;
        case 'B': return kGlyphB;
        case 'C': return kGlyphC;
        case 'D': return kGlyphD;
        case 'E': return kGlyphE;
        case 'F': return kGlyphF;
        case 'G': return kGlyphG;
        case 'H': return kGlyphH;
        case 'I': return kGlyphI;
        case 'J': return kGlyphJ;
        case 'K': return kGlyphK;
        case 'L': return kGlyphL;
        case 'M': return kGlyphM;
        case 'N': return kGlyphN;
        case 'O': return kGlyphO;
        case 'P': return kGlyphP;
        case 'Q': return kGlyphQ;
        case 'R': return kGlyphR;
        case 'S': return kGlyphS;
        case 'T': return kGlyphT;
        case 'U': return kGlyphU;
        case 'V': return kGlyphV;
        case 'W': return kGlyphW;
        case 'X': return kGlyphX;
        case 'Y': return kGlyphY;
        case 'Z': return kGlyphZ;
        default: return kGlyphSpace;
    }
}

static esp_err_t transmit_prefixed(uint8_t prefix, const uint8_t *data, size_t length)
{
    if (s_dev_handle == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t buffer[17] = {0};
    buffer[0] = prefix;

    size_t offset = 0;
    while (offset < length) {
        size_t chunk = length - offset;
        if (chunk > 16) {
            chunk = 16;
        }

        memcpy(&buffer[1], &data[offset], chunk);
        esp_err_t err = i2c_master_transmit(s_dev_handle, buffer, chunk + 1, -1);
        if (err != ESP_OK) {
            return err;
        }
        offset += chunk;
    }

    return ESP_OK;
}

static esp_err_t send_command(uint8_t command)
{
    return transmit_prefixed(0x00, &command, 1);
}

static esp_err_t update_display()
{
    ESP_RETURN_ON_ERROR(send_command(0x21), TAG, "Failed to set column address mode");
    ESP_RETURN_ON_ERROR(send_command(0x00), TAG, "Failed to set column start");
    ESP_RETURN_ON_ERROR(send_command(kDisplayWidth - 1), TAG, "Failed to set column end");
    ESP_RETURN_ON_ERROR(send_command(0x22), TAG, "Failed to set page address mode");
    ESP_RETURN_ON_ERROR(send_command(0x00), TAG, "Failed to set page start");
    ESP_RETURN_ON_ERROR(send_command(kPageCount - 1), TAG, "Failed to set page end");
    return transmit_prefixed(0x40, s_framebuffer, sizeof(s_framebuffer));
}

static void clear_buffer()
{
    memset(s_framebuffer, 0, sizeof(s_framebuffer));
}

static void draw_char(int x, int y, char c)
{
    const uint8_t *glyph = lookup_glyph(c);
    for (int col = 0; col < 5; ++col) {
        int draw_x = x + col;
        if (draw_x < 0 || draw_x >= kDisplayWidth) {
            continue;
        }

        for (int row = 0; row < 7; ++row) {
            if ((glyph[col] & (1U << row)) == 0) {
                continue;
            }

            int draw_y = y + row;
            if (draw_y < 0 || draw_y >= kDisplayHeight) {
                continue;
            }

            size_t index = static_cast<size_t>(draw_x) +
                           (static_cast<size_t>(draw_y) / 8U) * kDisplayWidth;
            s_framebuffer[index] |= static_cast<uint8_t>(1U << (draw_y & 0x07));
        }
    }
}

static void draw_text(int x, int y, const char *text)
{
    if (text == nullptr) {
        return;
    }

    int cursor_x = x;
    for (size_t i = 0; text[i] != '\0'; ++i) {
        draw_char(cursor_x, y, text[i]);
        cursor_x += 6;
        if (cursor_x >= kDisplayWidth) {
            break;
        }
    }
}

static esp_err_t write_line(uint8_t page, const char *text)
{
    char buffer[22] = {0};
    if (text != nullptr) {
        snprintf(buffer, sizeof(buffer), "%-21.21s", text);
    }

    draw_text(0, static_cast<int>(page) * 8, buffer);
    return ESP_OK;
}

esp_err_t init(int sda_gpio, int scl_gpio, int reset_gpio)
{
    if (s_ready) {
        return ESP_OK;
    }

    i2c_master_bus_handle_t bus_handle = nullptr;
    ESP_RETURN_ON_ERROR(app_i2c::acquire_bus(sda_gpio, scl_gpio, &bus_handle),
                        TAG,
                        "Failed to acquire shared I2C bus");

    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = kI2cAddress;
    dev_cfg.scl_speed_hz = 400000;

    esp_err_t err = i2c_master_bus_add_device(bus_handle, &dev_cfg, &s_dev_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_master_bus_add_device failed: %s", esp_err_to_name(err));
        return err;
    }

    static_cast<void>(reset_gpio);

    const uint8_t init_sequence[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12,
        0x81, 0xFF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6,
        0x2E, 0xAF,
    };

    ESP_LOGI(TAG, "Initializing SSD1306 panel (128x64)...");
    for (size_t i = 0; i < sizeof(init_sequence); ++i) {
        ESP_RETURN_ON_ERROR(send_command(init_sequence[i]), TAG, "Failed to send init command");
    }

    clear_buffer();
    ESP_RETURN_ON_ERROR(update_display(), TAG, "Failed to clear OLED after init");
    s_ready = true;
    return ESP_OK;
}

esp_err_t showStartup()
{
    if (ensure_ready() != ESP_OK) {
        return ESP_ERR_INVALID_STATE;
    }

    clear_buffer();
    write_line(0, "ESP-IDF Framework");
    write_line(2, "OLED ready");
    write_line(4, "Waiting for data...");
    return update_display();
}

esp_err_t showMessage(const char *line1, const char *line2)
{
    if (ensure_ready() != ESP_OK) {
        return ESP_ERR_INVALID_STATE;
    }

    clear_buffer();
    write_line(1, line1);
    if (line2 != nullptr) {
        write_line(3, line2);
    }
    return update_display();
}

esp_err_t showSensorData(const SensorDisplayData &data)
{
    if (ensure_ready() != ESP_OK) {
        return ESP_ERR_INVALID_STATE;
    }

    char line[22];
    char value_text[12];

    clear_buffer();

    format_scaled_100(value_text, sizeof(value_text), data.temperatureC);
    snprintf(line, sizeof(line), "Temp: %s C", value_text);
    write_line(0, line);

    format_scaled_100(value_text, sizeof(value_text), data.humidityPercent);
    snprintf(line, sizeof(line), "Hum : %s %%", value_text);
    write_line(2, line);

    format_scaled_100(value_text, sizeof(value_text), data.pressureHpa);
    snprintf(line, sizeof(line), "Pres: %s hPa", value_text);
    write_line(4, line);

    write_line(6, "MQTT publish active");
    return update_display();
}

}  // namespace oled_app
