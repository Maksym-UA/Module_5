#pragma once

#include "driver/gpio.h"
#include "esp_err.h"

class Application
{
    public:
        void start();

    private:
        static constexpr gpio_num_t kPhotoResistorPin = GPIO_NUM_4;
        static constexpr gpio_num_t kLed1Pin = GPIO_NUM_1;
        static constexpr gpio_num_t kLed2Pin = GPIO_NUM_2;
        static constexpr gpio_num_t kLed3Pin = GPIO_NUM_5;
        static constexpr gpio_num_t kLed4Pin = GPIO_NUM_6;
        static constexpr gpio_num_t kLed5Pin = GPIO_NUM_7;


        void run();
    };