# Tasks

Apply after `fix-stale-data-and-clock`. "Builds clean" means `idf.py build` shows no warnings from `main/`. Tick the matching AUDIT.md item in the commit.

## 1. Backlight driver (AUDIT item 22)

- [x] 1.1 In `ST7789.c`, use `LEDC_ResolutionRatio` for `.duty_resolution`, rename the LEDC macros to `BK_LEDC_TIMER`, `BK_LEDC_CHANNEL` and `BK_LEDC_MODE`, make `BK_Init()` `static`, and remove its prototype from `ST7789.h`. Verify: builds clean, `grep -rn 'LEDC_HS_\|LEDC_LS_MODE' main` finds nothing, and all six backlight levels still work on the board.

## 2. Config (AUDIT item 23)

- [x] 2.1 Delete the three `CONFIG_SPIRAM*` lines and `CONFIG_LV_FONT_MONTSERRAT_40=y` from `sdkconfig.defaults`, and add `CONFIG_IDF_TARGET="esp32c6"`. Found during apply: sdkconfig.defaults didn't name the target, so a deleted `sdkconfig` regenerated for the default esp32. Don't delete `sdkconfig`; set the font line in it to not-set by hand. Verify: the build log has no `unknown kconfig symbol` notes, and the app binary size doesn't grow. _(No notes; app 0x1a67e0 vs 0x1a6830, 80 B smaller; fresh sdkconfig regenerates as esp32c6.)_

## 3. Logs (AUDIT item 24)

- [x] 3.1 Demote the `MQTT_EVENT_DATA`, `TOPIC=` and `DATA=` lines and the default-event line to `ESP_LOGD`, and log `MQTT_EVENT_ERROR` at WARN with the error type (design decision 2). Add a one-line note to the AGENTS.md Debugging Tips on raising the `MQTT` tag to DEBUG. Verify: a 30 s boot log has exactly one INFO line per MQTT message (the parsed value), no raw `TOPIC=`/`DATA=` lines, and still shows `MQTT_EVENT_CONNECTED`. _(The original check, "fewer than 60 `MQTT:` lines", assumed night-time traffic; in daylight the light sensor alone sends ~80 messages/30 s. Board 1 on 2026-09-26: 114 `MQTT:` lines = connect + 3 status + 110 value lines for 110 messages, 0 raw lines; the same traffic would have logged ~440 lines before.)_

## 4. Comments (AUDIT item 25)

- [x] 4.1 Fix the comments at `weather_station_ui.c` (night-mode block), `main.c` (SD/SPI bus order) and `LVGL_Driver.h:13`, and drop `#include <sys/time.h>`. Verify: builds clean, and `grep -n 'saved brightness\|22:00 to 08:00\|behind the LCD\|!!!!!' main -r` finds nothing.

## 5. Integration

- [x] 5.1 Flash one board and check the boot log and screen are unchanged apart from the shorter log. Tick AUDIT items 22–25 with the commit. Verify: no `E (` lines, and all values are shown.
