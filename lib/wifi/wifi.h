#pragma once

#include <stdint.h>

extern int isConnected;

void wifi_init_sta(void);
void send_to_thingspeak(int32_t field1, int32_t field2);
