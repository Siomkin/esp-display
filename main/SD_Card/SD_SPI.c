#include "SD_SPI.h"

static const char *SD_TAG = "SD";

uint32_t Flash_Size = 0;

// Initializes the SPI bus shared with the LCD. The SD card itself is not mounted (unused);
// its CS is held high so the card ignores LCD traffic on the shared MOSI/SCLK lines.
void SD_Init(void)
{
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LVGL_BUF_LEN * sizeof(uint16_t),  // bus is shared with the LCD: fit one RGB565 draw buffer per transaction
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    gpio_config_t cs_cfg = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_NUM_CS,
    };
    ESP_ERROR_CHECK(gpio_config(&cs_cfg));
    gpio_set_level(PIN_NUM_CS, 1);
    ESP_LOGI(SD_TAG, "SPI bus initialized (SD card not mounted)");
}

void Flash_Searching(void)
{
    if(esp_flash_get_physical_size(NULL, &Flash_Size) == ESP_OK)
    {
        Flash_Size = Flash_Size / (uint32_t)(1024 * 1024);
        printf("Flash size: %ld MB\n", Flash_Size);
    }
    else{
        printf("Get flash size failed\n");
    }
}
