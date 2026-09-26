# Proposal

## Why

`AUDIT.md` items 22–25 are small maintenance traps:
- **Backlight PWM:** the resolution is defined in two places that must agree.
- **Config:** four dead lines in `sdkconfig.defaults` suggest the board has PSRAM.
- **Serial log:** 159 MQTT lines in a 30 s boot log bury the lines that matter.
- **Comments:** several are stale or wrong, one telling readers the SPI bus goes "behind" the LCD when it must go first.

## What Changes

- **Backlight driver:** `BK_Init()` uses `LEDC_ResolutionRatio`. The LEDC macros are renamed `BK_LEDC_TIMER`, `BK_LEDC_CHANNEL` and `BK_LEDC_MODE`, and `BK_Init()` becomes `static` (item 22).
- **Config:** remove `CONFIG_SPIRAM*` (3 lines) and `CONFIG_LV_FONT_MONTSERRAT_40` from `sdkconfig.defaults` (item 23). Also pin `CONFIG_IDF_TARGET="esp32c6"` there: without it, a deleted `sdkconfig` silently regenerates for the ESP32 (found during apply).
- **Logs:** the raw MQTT event, topic and payload lines and the default-event line move to DEBUG level. `MQTT_EVENT_ERROR` logs at WARN with the error type (item 24).
- **Comments:** fix the stale comments and drop the unused `<sys/time.h>` include (item 25).

No behaviour change on screen. The serial log at the default INFO level gets shorter.

**Depends on** `fix-stale-data-and-clock`, which edits the same UI and MQTT functions. Apply this after it.

## Capabilities

### New Capabilities
None.

### Modified Capabilities
None. This is a refactor with log and config tidy-up and no spec-level behaviour change, so `skip_specs: true` is set.

## Impact

- `main/LCD_Driver/ST7789.c` and `.h`, `main/Wireless/mqtt_handler.c`, `main/LVGL_UI/weather_station_ui.c`, `main/main.c`, `main/LVGL_Driver/LVGL_Driver.h`, `sdkconfig.defaults`.
- The local `sdkconfig` must be regenerated for the config change to show (`idf.py fullclean` or delete `sdkconfig`). It's gitignored.
