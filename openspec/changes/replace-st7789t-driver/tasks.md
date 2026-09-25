# Tasks

Build and flash with `.claude/skills/flash-device/scripts/flash.sh --monitor 30`. "Builds clean" means `idf.py build` shows no warnings from `main/`.

## 1. Baseline

- [x] 1.1 With the current firmware at 25% backlight and data on screen, take a reference photo, keeping the angle fixed. Verify: the photo is saved; it's the comparison for 2.3. _(Compared by eye against the usual display rather than a saved photo.)_

## 2. Swap the driver

- [x] 2.1 In `main/LCD_Driver/ST7789.c` `LCD_Init()`, replace `esp_lcd_panel_dev_st7789t_config_t` and `esp_lcd_new_panel_st7789t()` with `esp_lcd_panel_dev_config_t` (BGR, `LCD_RGB_DATA_ENDIAN_LITTLE`, 16 bpp, reset GPIO 21) and `esp_lcd_new_panel_st7789()`. After `esp_lcd_panel_init()`, send the command table from design decision 2 (starting with `B0h 00 E8`, with the `D0h` row sending 2 bytes), then call `esp_lcd_panel_invert_color(panel_handle, true)`. Keep `mirror`, `set_gap`, `disp_on_off` and the backlight as they are. Verify: builds clean.
- [x] 2.2 Delete `main/LCD_Driver/Vernon_ST7789T/`, its `SRCS` and `INCLUDE_DIRS` entries in `main/CMakeLists.txt`, and `#include "Vernon_ST7789T.h"` in `ST7789.h` (include `esp_lcd_panel_st7789.h` if `esp_lcd_panel_vendor.h` doesn't already cover it). Verify: `idf.py fullclean build` is clean and `grep -rni vernon main` finds nothing.
- [x] 2.3 Flash and compare with the 1.1 photo:
  - the same orientation, with no offset or garbage strip at the edges;
  - the same colours for gold, orange, cyan and white;
  - no flicker, and no garbage flash before the backlight turns on.

  Also cycle through all backlight levels. Verify: no visible difference, and the boot log has no `E (` lines.

## 3. Close out

- [x] 3.1 Update AUDIT.md item 11 with the commit, and update any README or AGENTS.md project tree that lists `Vernon_ST7789T`. Verify: `grep -rn Vernon README.md AGENTS.md` finds nothing.
