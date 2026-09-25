#include "button_handler.h"
#include "weather_station_ui.h"
#include "esp_lvgl_port.h"
#include "RGB.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "BUTTON";

static TaskHandle_t button_task_handle = NULL;

// Only wake the task: the level read here is unreliable while the contacts bounce
static void IRAM_ATTR button_isr_handler(void* arg)
{
    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(button_task_handle, &woken);
    portYIELD_FROM_ISR(woken);
}

static void button_task(void* arg)
{
    bool pressed = false;  // Last stable state (active-low: level 0 = pressed)
    int64_t press_start_ms = 0;

    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Let the contacts settle, drop the edges the bounce produced, then sample once
        vTaskDelay(pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS));
        ulTaskNotifyTake(pdTRUE, 0);
        bool now_pressed = gpio_get_level(BUTTON_BOOT_GPIO) == 0;
        if (now_pressed == pressed) {
            continue;  // Bounce only, no real state change
        }
        pressed = now_pressed;
        int64_t now_ms = esp_timer_get_time() / 1000;

        if (pressed) {
            press_start_ms = now_ms;
            ESP_LOGI(TAG, "Button pressed");
            continue;
        }

        int64_t press_duration = now_ms - press_start_ms;
        // Same lock as the UI update: otherwise it can override the new backlight level mid-update
        lvgl_port_lock(0);
        if (press_duration >= BUTTON_LONG_PRESS_MS) {
            // Long press - toggle RGB LED
            ESP_LOGI(TAG, "Long press detected (%lld ms) - Toggling RGB LED", press_duration);
            RGB_Toggle();
        } else {
            // Short press - cycle backlight brightness
            ESP_LOGI(TAG, "Short press detected (%lld ms) - Cycling backlight", press_duration);
            weather_station_cycle_backlight();
        }
        lvgl_port_unlock();
    }
}

esp_err_t button_handler_init(void)
{
    // Create the task first so the ISR always has a handle to notify
    if (xTaskCreate(button_task, "button_task", 2048, NULL, 10, &button_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create button task");
        return ESP_FAIL;
    }

    // Configure BOOT button with interrupt on both edges
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_ANYEDGE,  // Trigger on both rising and falling edge
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BUTTON_BOOT_GPIO),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Install ISR service
    ESP_ERROR_CHECK(gpio_install_isr_service(0));

    // Attach interrupt handler
    ESP_ERROR_CHECK(gpio_isr_handler_add(BUTTON_BOOT_GPIO, button_isr_handler, NULL));

    ESP_LOGI(TAG, "Button handler initialized");
    ESP_LOGI(TAG, "GPIO %d (BOOT): Short press = Cycle backlight (5%%->10%%->25%%->100%%->0%%->1%%->5%%), Long press = RGB LED on/off", BUTTON_BOOT_GPIO);

    return ESP_OK;
}
