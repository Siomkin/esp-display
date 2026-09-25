#pragma once

#include "driver/gpio.h"
#include "led_strip.h"
#include "stdbool.h"

#define BLINK_GPIO 8

void RGB_Init(void);
void RGB_Start(void);                   // Start animation task (idle, LED off, until enabled)
void RGB_Set_Enabled(bool enabled);
void RGB_Toggle(void);                  // Long press: flip on/off
