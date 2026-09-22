#pragma once
#include <stdio.h>
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"

#include "ST7789.h"

#define LVGL_BUF_LEN  (EXAMPLE_LCD_H_RES * 20)   // pixels per draw buffer

// Starts the esp_lvgl_port task. LVGL calls from other tasks must hold lvgl_port_lock()/lvgl_port_unlock().
void LVGL_Init(void);                     // Call this function to initialize the screen (must be called in the main function) !!!!!
