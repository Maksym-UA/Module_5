#pragma once

#include "driver/gpio.h"
#include "esp_err.h"

void application_init();

class Application
{
    public:
        void start();

    private:
        void run();
    };