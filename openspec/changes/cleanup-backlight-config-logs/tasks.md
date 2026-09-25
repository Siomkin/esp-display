# Tasks

Apply after `fix-stale-data-and-clock`. "Builds clean" means `idf.py build` shows no warnings from `main/`. Tick the matching AUDIT.md item in the commit.

## 1. Backlight driver (AUDIT item 22)

- [ ] 1.1 In `ST7789.c`, use `LEDC_ResolutionRatio` for `.duty_resolution`, rename the LEDC macros to `BK_LEDC_TIMER`, `BK_LEDC_CHANNEL` and `BK_LEDC_MODE`, make `BK_Init()` `static`, and remove its prototype from `ST7789.h`. Verify: builds clean, `grep -rn 'LEDC_HS_\|LEDC_LS_MODE' main` finds nothing, and all six backlight levels still work on the board.

## 2. Config (AUDIT item 23)

- [ ] 2.1 Delete the three `CONFIG_SPIRAM*` lines and `CONFIG_LV_FONT_MONTSERRAT_40=y` from `sdkconfig.defaults`, then regenerate `sdkconfig` with `idf.py fullclean build`. Verify: the build log has no `unknown kconfig symbol` notes, and the app binary size doesn't grow.

## 3. Logs (AUDIT item 24)

- [ ] 3.1 Demote the `MQTT_EVENT_DATA`, `TOPIC=` and `DATA=` lines and the default-event line to `ESP_LOGD`, and log `MQTT_EVENT_ERROR` at WARN with the error type (design decision 2). Add a one-line note to the AGENTS.md Debugging Tips on raising the `MQTT` tag to DEBUG. Verify: a 30 s boot log has fewer than 60 `MQTT:` lines and still shows `MQTT_EVENT_CONNECTED` and the parsed values.

## 4. Comments (AUDIT item 25)

- [ ] 4.1 Fix the comments at `weather_station_ui.c` (night-mode block), `main.c` (SD/SPI bus order) and `LVGL_Driver.h:13`, and drop `#include <sys/time.h>`. Verify: builds clean, and `grep -n 'saved brightness\|22:00 to 08:00\|behind the LCD\|!!!!!' main -r` finds nothing.

## 5. Integration

- [ ] 5.1 Flash one board and check the boot log and screen are unchanged apart from the shorter log. Tick AUDIT items 22–25 with the commit. Verify: no `E (` lines, and all values are shown.
