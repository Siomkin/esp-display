#pragma once

#include "mqtt_client.h"  // ESP-IDF MQTT client
#include "esp_err.h"
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "app_config.h"  // User settings (config/app_config.h, gitignored)

/* A value with no update for this long is reported as not valid */
#define SENSOR_STALE_MS  (5 * 60 * 1000)   // Sensors publish at least once a minute
#define CLOCK_STALE_MS   (60 * 60 * 1000)  // Clock counts on its own in between (see weather_station_ui.c)

/* Data structure for sensor readings */
typedef struct {
    float temp_outside;
    float temp_inside;
    float humidity;
    float illuminance;
    char time_str[16]; // HH:MM
    char date_str[16]; // YYYY-MM-DD
    bool temp_outside_valid;
    bool temp_inside_valid;
    bool humidity_valid;
    bool illuminance_valid;
    bool time_valid;
    bool date_valid;
    // Tick of the last update of each value (staleness; time_tick also drives the counting clock)
    TickType_t temp_outside_tick;
    TickType_t temp_inside_tick;
    TickType_t humidity_tick;
    TickType_t illuminance_tick;
    TickType_t time_tick;
    TickType_t date_tick;
} sensor_data_t;

/**
 * @brief Initialize MQTT client and connect to broker
 * @return ESP_OK on success
 */
esp_err_t mqtt_client_init(void);

/**
 * @brief Get the latest sensor data
 * Values older than SENSOR_STALE_MS (time/date: CLOCK_STALE_MS) come back with _valid = false
 * @param data Pointer to sensor_data_t structure to fill
 */
void mqtt_get_sensor_data(sensor_data_t *data);
