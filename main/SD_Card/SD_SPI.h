
#pragma once

#include "esp_flash.h"

#include "ST7789.h"

#define PIN_NUM_MOSI    EXAMPLE_PIN_NUM_MOSI
#define PIN_NUM_MISO    5
#define PIN_NUM_SCLK    EXAMPLE_PIN_NUM_SCLK
#define PIN_NUM_CS      4

void SD_Init(void);                 // SPI bus init (shared with LCD); SD card is not mounted
void Flash_Searching(void);
