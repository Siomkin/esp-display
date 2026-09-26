#pragma once
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/ledc.h"

// LCD SPI GPIO
// Using SPI2 
#define LCD_HOST  SPI2_HOST

#define EXAMPLE_LCD_PIXEL_CLOCK_HZ     (40 * 1000 * 1000)   // was 12MHz; drop to 20MHz if you see artifacts (SCLK/MOSI go via GPIO matrix)
#define EXAMPLE_PIN_NUM_SCLK           7
#define EXAMPLE_PIN_NUM_MOSI           6
#define EXAMPLE_PIN_NUM_LCD_CS         14
#define EXAMPLE_PIN_NUM_LCD_DC         15
#define EXAMPLE_PIN_NUM_LCD_RST        21
#define EXAMPLE_PIN_NUM_BK_LIGHT       22
// The pixel number in horizontal and vertical
#define EXAMPLE_LCD_H_RES              172
#define EXAMPLE_LCD_V_RES              320
// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS           8
#define EXAMPLE_LCD_PARAM_BITS         8

#define Offset_X 0
#define Offset_Y 34


// Backlight PWM (LEDC low-speed channel)
#define BK_LEDC_TIMER          LEDC_TIMER_0
#define BK_LEDC_MODE           LEDC_LOW_SPEED_MODE
#define BK_LEDC_CHANNEL        LEDC_CHANNEL_0
#define LEDC_ResolutionRatio   LEDC_TIMER_13_BIT
#define LEDC_MAX_Duty          ((1 << LEDC_ResolutionRatio) - 1)


extern esp_lcd_panel_handle_t panel_handle;
extern esp_lcd_panel_io_handle_t io_handle;

void BK_Light(uint8_t Light);                   // Call this function to adjust the brightness of the backlight. The value of the parameter Light ranges from 0 to 100

void LCD_Init(void);                     // Panel + backlight PWM; call after SD_Init() (creates the SPI bus)