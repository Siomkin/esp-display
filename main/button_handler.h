#pragma once

#include "driver/gpio.h"
#include "esp_err.h"

// Button GPIO pin for ESP32-C6-LCD-1.47
#define BUTTON_BOOT_GPIO       GPIO_NUM_9   // BOOT button on ESP32-C6

// Button timing configuration
#define BUTTON_DEBOUNCE_MS     50           // Debounce time
#define BUTTON_LONG_PRESS_MS   1000         // Long press threshold

/**
 * @brief Initialize button handlers
 * Sets up GPIO interrupts for button presses
 * 
 * Button behavior:
 * - Short press: Cycle backlight brightness (5% -> 10% -> 25% -> 100% (auto night mode) -> 0% -> 1% -> 5%)
 * - Long press (>1s): Toggle RGB LED on/off
 */
esp_err_t button_handler_init(void);
