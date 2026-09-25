#pragma once

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

/* WiFi credentials from config/app_config.h (gitignored) */
#include "app_config.h"

/* How long boot waits for the first association before continuing */
#define WIFI_CONNECT_TIMEOUT_MS  30000

/* Event bits */
#define WIFI_CONNECTED_BIT BIT0

/**
 * @brief Initialize WiFi and start connecting to the configured network.
 *        Reconnects indefinitely after disconnects (router reboot, AP firmware update).
 * @return ESP_OK if connected within WIFI_CONNECT_TIMEOUT_MS, ESP_ERR_TIMEOUT otherwise
 *         (background reconnect continues either way)
 */
esp_err_t wifi_connect_init(void);
