# Tasks

Apply after `fix-audit-behavior` is merged. Build after every task with `docker run --rm -v "$PWD":/project -w /project espressif/idf:v6.1 idf.py build`; "builds clean" means no new warnings. Tick the matching AUDIT.md item in each task's commit.

## 1. Dead code (AUDIT items 12–16)

- [ ] 1.1 Delete `weather_station_toggle_display()`, its prototype, and `is_display_enabled` (item 12). Verify: builds clean, and `grep -rn toggle_display main` finds nothing.
- [ ] 1.2 Delete `wifi_is_connected()`, `wifi_get_event_group()`, `WIFI_FAIL_BIT` and its clear, `s_is_connected` if unused after that, `mqtt_is_connected()` and `mqtt_connected` (item 13). Keep `s_wifi_event_group` and `WIFI_CONNECTED_BIT`. Verify: builds clean, and WiFi reconnects after an AP reboot (boot log shows `Got IP` again).
- [ ] 1.3 In `ST7789.h` and `ST7789.c`, delete `EXAMPLE_LCD_BK_LIGHT_ON_LEVEL` and `_OFF_LEVEL`, `LEDC_HS_CH0_GPIO`, `LEDC_TEST_DUTY`, the commented-out SPI bus block, and `ledc_fade_func_install(0)` (item 14). Verify: builds clean, and the backlight still cycles through every level.
- [ ] 1.4 In `Vernon_ST7789T.c`, delete the commented-out `printf` lines and the commented-out MADCTL and COLMOD lines and the bare `0x2C` write (the audit's "duplicate `break;`" was a misread; there is none). Relabel the `0x21` comment "Display Inversion On" (item 15). Verify: builds clean, and the display shows correct colours after boot.
- [ ] 1.5 Make `Flash_Size` a local variable in `Flash_Searching()` and remove its `extern`. Make `Set_RGB()` `static` and remove its prototype (item 16). Verify: builds clean, and the boot log still prints `Flash size: N MB`.

## 2. Backlight table (AUDIT item 8)

- [ ] 2.1 Replace the `switch` in `weather_station_cycle_backlight()` with the `levels[]` table from design decision 2. Delete `auto_brightness_enabled` and `saved_brightness`, add `weather_station_backlight_on()`, and call it from `main.c` in place of `BK_Light(5)`. Verify: builds clean, boot brightness is 5%, six short presses step through 10/25/100/0/1/5, and night mode still dims at level 100.
- [ ] 2.2 Update the AGENTS.md "Changing Brightness Levels" section to "edit `levels[]`". Verify that the section names only symbols that exist.

## 3. RGB (AUDIT items 9, 10)

- [ ] 3.1 On the host, check that the formula in design decision 3 reproduces all 192 `RGB_Data` rows ×3 (a Python one-liner against the table text). Then replace the table and the lookup in `rgb_task_fn`. Verify: builds clean, and a long press shows the same smooth rainbow.
- [ ] 3.2 Add `RGB_Toggle()` to `RGB.c` and `RGB.h`, call it from `button_handler.c`, and delete `weather_station_toggle_rgb()` and `is_rgb_enabled`. Update the AGENTS.md "RGB stuck on" tip. Verify: builds clean, and long presses toggle the LED on and off and leave it off after boot.

## 4. MQTT topic table (AUDIT item 7)

- [ ] 4.1 Implement the topic table from design decision 1 for the four float topics. Use it for both subscribe and parse, and keep date and time as explicit cases. Verify: builds clean, the boot log shows the same per-topic `ESP_LOGI` lines, and all six values appear on screen.
- [ ] 4.2 Update the AGENTS.md "Adding New MQTT Topic" steps: the config define, one table row, a `sensor_data_t` field, and the UI. Verify the steps against the code.

## 5. Includes (AUDIT item 17)

- [ ] 5.1 Drop `lvgl.h` and `LVGL_Driver.h` from `ST7789.h` and add `#include "LVGL_Driver.h"` to `main.c` and `SD_SPI.c`. Drop `mqtt_client.h` from `weather_station_ui.h` and fix its "pressure" comment. Verify: a clean full build (`idf.py fullclean build`) with no new warnings.

## 6. Integration check

- [ ] 6.1 Flash with `.claude/skills/flash-device/scripts/flash.sh --monitor 30` and run the AGENTS.md Testing Checklist. Verify all items pass and free heap in the boot log is no lower than before this change.
