#include "ST7789.h"

static const char *TAG_LCD = "WS_LCD";

esp_lcd_panel_handle_t panel_handle = NULL;
esp_lcd_panel_io_handle_t io_handle = NULL;

// Board-specific ST7789 setup, sent after the ESP-IDF driver's own init (SLPOUT/MADCTL/COLMOD/RAMCTRL)
static const struct {
    uint8_t cmd;
    uint8_t data[14];
    uint8_t len;
} panel_init_cmds[] = {
    {0xB0, {0x00, 0xE8}, 2},                    // RAMCTRL: little-endian, EPF=10 (IDF default sends 0xF8)
    {0xB2, {0x0C, 0x0C, 0x00, 0x33, 0x33}, 5},  // Porch setting
    {0xB7, {0x75}, 1},                          // Gate control, Vgh=13.65V, Vgl=-10.43V
    {0xBB, {0x1A}, 1},                          // VCOM=1.175V
    {0xC0, {0x80}, 1},                          // LCM control
    {0xC2, {0x01, 0xFF}, 2},                    // VDV and VRH command enable
    {0xC3, {0x13}, 1},                          // VRH set
    {0xC4, {0x20}, 1},                          // VDV set
    {0xC6, {0x0F}, 1},                          // Frame rate 60Hz
    {0xD0, {0xA4, 0xA1}, 2},                    // Power control 1: AVDD=6.8V, AVCL=-4.8V, VDDS=2.3V
    {0xE0, {0xD0, 0x0D, 0x14, 0x0D, 0x0D, 0x09, 0x38, 0x44, 0x4E, 0x3A, 0x17, 0x18, 0x2F, 0x30}, 14},  // Positive gamma
    {0xE1, {0xD0, 0x09, 0x0F, 0x08, 0x07, 0x14, 0x37, 0x44, 0x4D, 0x38, 0x15, 0x16, 0x2C, 0x2E}, 14},  // Negative gamma
};

void LCD_Init(void)
{
    ESP_LOGI(TAG_LCD, "Install panel IO");                                              
    esp_lcd_panel_io_spi_config_t io_config = {                                             
        .dc_gpio_num = EXAMPLE_PIN_NUM_LCD_DC,
        .cs_gpio_num = EXAMPLE_PIN_NUM_LCD_CS,
        .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
        .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,  // matches swap_bytes = false in LVGL_Driver.c
        .bits_per_pixel = 16,
    };
    ESP_LOGI(TAG_LCD, "Install ST7789 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    for (size_t i = 0; i < sizeof(panel_init_cmds) / sizeof(panel_init_cmds[0]); i++) {
        ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, panel_init_cmds[i].cmd,
                                                  panel_init_cmds[i].data, panel_init_cmds[i].len));
    }
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, Offset_X, Offset_Y));  // 172px panel sits at offset 34 in 240px GRAM

    // user can flush pre-defined pattern to the screen before we turn on the screen or backlight
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    // Backlight off: GRAM holds garbage until LVGL flushes the first frame (main turns it on)
    BK_Init();                                                                                          // Initialize the backlight
    BK_Light(0);

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Backlight program
static ledc_channel_config_t ledc_channel;
void BK_Init(void)
{
    ESP_LOGI(TAG_LCD, "Turn off LCD backlight");
    // No gpio_config here: LEDC routes the pin itself (IDF 6 warns if the GPIO is already reserved)
    // 配置LEDC
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 5000,
        .speed_mode = LEDC_LS_MODE,
        .timer_num = LEDC_HS_TIMER,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel.channel    = LEDC_HS_CH0_CHANNEL;
    ledc_channel.duty       = 0;
    ledc_channel.gpio_num   = EXAMPLE_PIN_NUM_BK_LIGHT;
    ledc_channel.speed_mode = LEDC_LS_MODE;
    ledc_channel.timer_sel  = LEDC_HS_TIMER;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}
void BK_Light(uint8_t Light)
{   
    if(Light > 100) Light = 100;
    uint16_t Duty = LEDC_MAX_Duty * Light / 100;  // 0 -> off, 1 -> 81/8191
    // 设置PWM占空比
    ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, Duty);
    ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
}
// end Backlight program