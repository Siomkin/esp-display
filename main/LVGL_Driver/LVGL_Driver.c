#include "LVGL_Driver.h"

static const char *TAG_LVGL = "WS_LVGL";

void LVGL_Init(void)
{
    ESP_LOGI(TAG_LVGL, "Initialize LVGL port");
    lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    port_cfg.timer_period_ms = 10;  // tick ISR period; no animations, so 10ms resolution is plenty
    ESP_ERROR_CHECK(lvgl_port_init(&port_cfg));

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = LVGL_BUF_LEN,
        .double_buffer = true,
        .hres = EXAMPLE_LCD_V_RES,  // 320, landscape
        .vres = EXAMPLE_LCD_H_RES,  // 172
        .rotation = {
            .swap_xy = true,
            .mirror_x = true,
            .mirror_y = true,
        },
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags = {
            .buff_dma = true,
            .swap_bytes = false,    // panel RAMCTRL (0xB0) is little-endian
        },
    };
    if (lvgl_port_add_disp(&disp_cfg) == NULL) {
        ESP_LOGE(TAG_LVGL, "Failed to add LVGL display");
    }
}
