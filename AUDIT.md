# Code Audit

Tick an item when it lands, and note the commit next to it.

- P0: broken today, the behaviour is wrong.
- P1: misleading today, users or contributors will get it wrong.
- P2: inconsistent with siblings or other layers, or costly to change.
- P3: polish, duplication, dead code.

## P0: Broken

None found.

## P1: Misleading

- [x] 1. (0e2c282) `MQTT_USERNAME` / `MQTT_PASSWORD` are documented in `config/app_config.h.example:25-26` ("Optional: leave empty if not required") but never read: `mqtt_cfg` in `main/Wireless/mqtt_handler.c:110-118` sets only URI and client ID. A broker that needs a login rejects the board, and nothing in the code shows why. Set `.credentials.username` and `.credentials.authentication.password` in `mqtt_cfg`, passing `NULL` when the string is empty (`MQTT_USERNAME[0] ? MQTT_USERNAME : NULL`).
- [x] 2. (0e2c282) The night-mode check `h >= NIGHT_MODE_START_HOUR || h < NIGHT_MODE_END_HOUR` (`main/LVGL_UI/weather_station_ui.c:266`) only works when the window crosses midnight. The config comment (`app_config.h.example:49`) allows any start and end, but with `START=1, END=6` every hour counts as night (24 of 24, checked), so the "auto" level stays at 1% all day. Use Decompose Conditional: `START <= END ? (h >= START && h < END) : (h >= START || h < END)`.

## P2: Inconsistent or costly to change

- [x] 3. (0e2c282) Race on the backlight state. `weather_station_cycle_backlight()` runs in the button task (`main/button_handler.c:52`, priority 10) and writes `backlight_level`, `auto_brightness_enabled` and `night_mode`, and calls `BK_Light()`. The LVGL task (priority 4) reads and writes the same state in `weather_station_ui_update()` (`weather_station_ui.c:264-270`), with no lock. If a press lands between the check at :264 and `BK_Light()` at :270, the LVGL task then overrides the user's new level, for example turning the panel back on after the user chose "off". Fix: in the button task, wrap both actions in `lvgl_port_lock(0)` / `lvgl_port_unlock()`, which is the project's rule for touching UI state from other tasks.
- [x] 4. (0e2c282) The button is dead for up to 30 s at boot while the AP is down. `button_handler_init()` runs after `wifi_connect_init()` (`main/main.c:52-65`), which blocks for up to `WIFI_CONNECT_TIMEOUT_MS` (30000, `wifi_connect.h:13`). Its result is only logged, since MQTT starts either way (main.c:59). Fix: move `button_handler_init()` above the WiFi call. Better still, drop the wait (see item 13). `Needs a decision.`
- [x] 5. (0e2c282) `BK_Light()` does not produce the percentages the rest of the code names (`main/LCD_Driver/ST7789.c:85`): `LEDC_MAX_Duty - 81*(100-Light)` uses 81 where 8191/100 ≈ 81.91 would be right. The measured duty is 1%→2.1%, 5%→6.1%, 10%→11.0%, 25%→25.8%, so "1% night mode" is really twice as bright as documented. Fix: `Duty = LEDC_MAX_Duty * Light / 100`. `Needs a decision.` This dims the low levels on real hardware, so check at night that 1% is still readable. If it isn't, change the level table instead of the formula.
- [x] 6. (0e2c282) The ST7789T init sends Power Control 1 (`0xD0`) with a two-byte array but a length of 1 (`main/LCD_Driver/Vernon_ST7789T/Vernon_ST7789T.c:193`), so `0xA1` is never sent and the "AVDD=6.8V, AVCL=-4.8V, VDDS=2.3V" in the comment is not what the panel gets. Fix: length `2`. `Needs a decision.` The panel looks fine today, so check the image on hardware before and after.
- [ ] 7. Adding a sensor topic is Shotgun Surgery. It touches the subscribe list (`mqtt_handler.c:27-32`), the `strcmp` chain (:61-85), two fields per value in `sensor_data_t` (`mqtt_handler.h:9-22`), the UI, and the config example; AGENTS.md documents this five-step recipe. Fix: a static table `{topic, float *value, bool *valid}` for the four float topics, looped for both subscribe and parse, with date and time kept as the two string special cases (Replace Conditional with a lookup table). This also removes the duplicated `atof` / `_valid = true` / log blocks.

## P3: Polish

### Simplify

- [ ] 8. `weather_station_cycle_backlight()` (`weather_station_ui.c:159-199`) is a six-case `switch` that repeats `BK_Light(n); auto_brightness_enabled = false; ESP_LOGI(...)` for each case. Two of its state variables are redundant:
  - `auto_brightness_enabled` is true exactly when `backlight_level == 5`, and :264 checks both.
  - `saved_brightness` is always 100 when it is read. The `= 25` at :188 is overwritten at :193 before :270 can read it, because auto mode is only reachable from level 5.

  Fix: `static const uint8_t levels[] = {0, 1, 5, 10, 25, 100};` then `BK_Light(levels[backlight_level])`, delete both variables, and use `BK_Light(is_night ? 1 : 100)`. Main's `BK_Light(5)` (`main.c:48`) and the "Matches the 5% set in main()" comment (:34) then become `levels[2]`, so the start level lives in one place. About 35 lines become about 10.
- [ ] 9. `RGB_Data[192][3]` (`main/RGB/RGB.c:5-32`) is a hand-written linear ramp with three segments (R→G, G→B, B→R, 64 steps each), and every entry is then multiplied by 3 at :81. Fix: compute the frame in `rgb_task_fn`, `seg = i / 64, k = i % 64`, then rotate `(64-k, k, 0)` by `seg` (Substitute Algorithm). About 28 lines become about 6.
- [ ] 10. The RGB on/off state lives twice: `is_rgb_enabled` (`weather_station_ui.c:33`) and `rgb_enabled` (`RGB.c:35`). `weather_station_toggle_rgb()` (:210-215) is a Middle Man between the button and `RGB_Set_Enabled()`. Fix: add `RGB_Toggle()` in `RGB.c`, call it from `button_handler.c:48`, and delete the UI wrapper and its flag. Update the AGENTS.md "RGB stuck on" tip, which names `is_rgb_enabled`.
- [ ] 11. `Vernon_ST7789T.c/.h` (~350 lines) is a copy of an old Espressif panel driver. Its constructor computes `madctl_val` and `colmod_cal` from the config (:71-98), but `init` ignores both and hard-codes `0x36=0x00` and `0x3A=0x55` (:171-173). BGR only takes effect through the later `mirror()` call in `ST7789.c:45`, and a `bits_per_pixel = 18` config would claim 24 bpp while the panel stays in RGB565. Fix: use ESP-IDF's built-in `esp_lcd_new_panel_st7789()`, then send the vendor register writes (B0/B2/B7/BB/C0-C6/D0/E0/E1) with `esp_lcd_panel_io_tx_param()` from `LCD_Init()`, and use `esp_lcd_panel_invert_color(panel, true)` for the `0x21`. `Needs a decision.` The built-in init writes RAMCTRL (`0xB0`) itself, and this board needs `{0x00, 0xE8}` (little-endian, see `LVGL_Driver.c:27`), so verify on hardware.

### Dead code

- [ ] 12. `weather_station_toggle_display()` and `is_display_enabled` (`weather_station_ui.c:32, 201-208`; `weather_station_ui.h:23-26`) have no caller in `main/`, the README, AGENTS.md or `.claude/`. Delete them. This also removes the UI's only use of the `panel_handle` global.
- [ ] 13. The WiFi and MQTT status API is unused. There are no callers of `wifi_is_connected()`, `wifi_get_event_group()` (`wifi_connect.c:140-148`, `.h:27-37`), `mqtt_is_connected()` or `mqtt_connected` (`mqtt_handler.c:12, 24, 39, 145-148`, `.h:36-40`). `WIFI_FAIL_BIT` is never set and only cleared (`wifi_connect.c:82`). Delete them. If item 4 drops the boot wait, `s_wifi_event_group`, both bits and `WIFI_CONNECT_TIMEOUT_MS` go as well.
- [ ] 14. Leftovers in `ST7789.h` and `ST7789.c`. None of these is referenced anywhere:
  - macros: `EXAMPLE_LCD_BK_LIGHT_ON_LEVEL` and `_OFF_LEVEL` (:23-24), `LEDC_HS_CH0_GPIO` (:44), `LEDC_TEST_DUTY` (:46);
  - the commented-out SPI bus block in `ST7789.c:10-19`;
  - `ledc_fade_func_install(0)` at `ST7789.c:80`, which installs the fade ISR and semaphore although no fade API is ever called.

  Delete them.
- [ ] 15. Leftovers in `Vernon_ST7789T.c`:
  - commented-out `printf("AAAA…")` (:115, :164) and commented-out MADCTL and COLMOD writes (:167-168);
  - a duplicate `break;` (:81);
  - a bare `0x2C` RAMWR with no data (:203);
  - the comment "Sleep Out" (:198) on `0x21`, which is Display Inversion On.

  Delete the dead lines and fix the comment. The last one misleads anyone tuning the init sequence.
- [ ] 16. `Flash_Size` is an exported global (`SD_SPI.h:13`, `SD_SPI.c:5`) that is only used inside `Flash_Searching()`, and that function has nothing to do with the SD card or the SPI bus. Make it a local variable, and move the one-line log into `main.c` or delete it. `Set_RGB()` (`RGB.h:10`) is likewise only called inside `RGB.c`, so make it `static`.

### Headers

- [ ] 17. `ST7789.h:17` includes `LVGL_Driver.h`, and `LVGL_Driver.h:8` includes `ST7789.h`; only `#pragma once` stops the loop. `SD_SPI.h` pulls in the whole LCD and LVGL header chain just for two pin numbers and `LVGL_BUF_LEN` (`SD_SPI.c:17`). `weather_station_ui.h:4` includes `mqtt_client.h` without using it, and its doc comment (:8) promises "pressure", which the UI doesn't show (it shows illuminance). Fix: drop the `LVGL_Driver.h` and `lvgl.h` includes from `ST7789.h`, since `ST7789.c` doesn't use LVGL. Then include `LVGL_Driver.h` directly in `main.c` (it needs `LVGL_Init` and `lvgl_port_lock`) and in `SD_SPI.c` (it needs `LVGL_BUF_LEN`). Drop `mqtt_client.h` and correct the comment.
