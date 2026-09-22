#pragma once

#include "driver/gpio.h"
#include "led_strip.h"
#include "stdbool.h"

#define BLINK_GPIO 8

void RGB_Init(void);
void Set_RGB( uint8_t red_val, uint8_t green_val, uint8_t blue_val);
void RGB_Start(void);                   // Start animation task (idle, LED off, until enabled)
void RGB_Set_Enabled(bool enabled);
