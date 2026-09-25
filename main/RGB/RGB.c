#include "RGB.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"


static led_strip_handle_t led_strip;
static volatile bool rgb_enabled = false;  // Off by default; toggled by long press
static TaskHandle_t rgb_task = NULL;


void RGB_Init(void)
{
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1, // at least one LED on board
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));

    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
}

static void Set_RGB(uint8_t red_val, uint8_t green_val, uint8_t blue_val)
{
    /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
    led_strip_set_pixel(led_strip, 0, red_val, green_val, blue_val);
    /* Refresh the strip to send data */
    led_strip_refresh(led_strip);
}

void RGB_Set_Enabled(bool enabled)
{
    rgb_enabled = enabled;
    if (rgb_task) {
        xTaskNotifyGive(rgb_task);  // Task owns the LED: it clears it or resumes the animation
    }
}

void RGB_Toggle(void)
{
    RGB_Set_Enabled(!rgb_enabled);
    ESP_LOGI("RGB", "RGB LED %s", rgb_enabled ? "Enabled" : "Disabled");
}

static void rgb_task_fn(void *arg)
{
    uint8_t i = 0;
    while (1) {
        if (!rgb_enabled) {
            Set_RGB(0, 0, 0);
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // Sleep until RGB_Set_Enabled() changes state
            continue;
        }
        // Rainbow R->G->B->R, 64 steps per segment; a fades out while b fades in
        uint8_t k = i % 64, a = (64 - k) * 3, b = (k + 1) * 3;
        switch (i / 64) {
            case 0:  Set_RGB(a, b, 0); break;
            case 1:  Set_RGB(0, a, b); break;
            default: Set_RGB(b, 0, a); break;
        }
        i = (i + 1) % 192;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void RGB_Start(void)
{
    xTaskCreate(rgb_task_fn, "rgb", 4096, NULL, 4, &rgb_task);
}
