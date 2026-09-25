#pragma once

#include "mqtt_client.h"  // ESP-IDF MQTT client
#include "esp_err.h"
#include <stdbool.h>
#include "app_config.h"  // User settings (config/app_config.h, gitignored)

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
} sensor_data_t;

/**
 * @brief Initialize MQTT client and connect to broker
 * @return ESP_OK on success
 */
esp_err_t mqtt_client_init(void);

/**
 * @brief Get the latest sensor data
 * @param data Pointer to sensor_data_t structure to fill
 */
void mqtt_get_sensor_data(sensor_data_t *data);
