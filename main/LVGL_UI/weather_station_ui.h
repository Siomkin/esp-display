#pragma once

#include "lvgl.h"
#include "mqtt_client.h"

/**
 * @brief Initialize the weather station UI
 * Creates a beautiful display showing time, date, temperature, humidity, and pressure
 */
void weather_station_ui_init(void);

/**
 * @brief Update the UI with latest sensor data
 * Should be called periodically to refresh the display
 */
void weather_station_ui_update(void);

/**
 * @brief Cycle backlight brightness levels
 */
void weather_station_cycle_backlight(void);

/**
 * @brief Turn the backlight on at the boot level (call once the first frame is on the panel)
 */
void weather_station_backlight_on(void);

