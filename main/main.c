/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "ST7789.h"
#include "SD_SPI.h"
#include "RGB.h"
#include "wifi_connect.h"
#include "mqtt_handler.h"
#include "weather_station_ui.h"
#include "button_handler.h"
#include "nvs_flash.h"
#include "esp_log.h"


static const char *TAG = "MAIN";



void app_main(void)
{
    // Initialize NVS (required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize hardware
    ESP_LOGI(TAG, "Initializing hardware...");
    Flash_Searching();
    RGB_Init();
    RGB_Start();                            // LED stays off until long press enables it
    SD_Init();                              // SD must be initialized behind the LCD
    LCD_Init();                             // Backlight stays off until the first frame is on the panel
    LVGL_Init();                            // Initialize LVGL

    // Build the UI before WiFi (which can block for seconds) and push one frame, then light the panel
    ESP_LOGI(TAG, "Initializing Weather Station UI...");
    lvgl_port_lock(0);  // LVGL runs in the esp_lvgl_port task
    weather_station_ui_init();
    lv_refr_now(NULL);  // Render and flush the first frame now
    lvgl_port_unlock();
    vTaskDelay(1);      // Double-buffered: last strip's DMA (~1.4ms) may still be in flight
    weather_station_backlight_on();  // Boot level (5%) lives in the UI's level table

    // Button only needs the UI and RGB: usable now, not after the WiFi wait (up to 30s)
    ESP_LOGI(TAG, "Initializing button handlers...");
    button_handler_init();

    // Initialize WiFi (keeps retrying forever if AP is down)
    ESP_LOGI(TAG, "Connecting to WiFi...");
    esp_err_t wifi_ret = wifi_connect_init();
    if (wifi_ret == ESP_OK) {
        ESP_LOGI(TAG, "WiFi connected successfully!");
    } else {
        ESP_LOGW(TAG, "WiFi not ready yet; reconnect continues in background");
    }

    // Start MQTT regardless — client auto-reconnects once WiFi is up
    ESP_LOGI(TAG, "Starting MQTT client...");
    mqtt_client_init();
    // app_main returns; LVGL, MQTT and button work continue in their own tasks
}
