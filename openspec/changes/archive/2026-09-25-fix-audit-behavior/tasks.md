# Tasks

Build and flash with `.claude/skills/flash-device/scripts/flash.sh --monitor 30`. The project has no unit-test harness, so each task is verified with a build that has no new warnings plus the named check on hardware or in the boot log.

## 1. MQTT broker login (AUDIT item 1)

- [x] 1.1 In `main/Wireless/mqtt_handler.c` `mqtt_client_init()`, set `.credentials.username` and `.credentials.authentication.password` from `MQTT_USERNAME` and `MQTT_PASSWORD`, using `NULL` when a string is empty. Verify: with the current empty credentials the boot log still shows `MQTT_EVENT_CONNECTED` and data arrives. _(Verified 2026-09-25: MQTT_EVENT_CONNECTED at 6.08 s, all six topics received.)_
- [x] 1.2 Verify the login path on a broker or test user that requires credentials: correct credentials connect, and a wrong password logs a connection error and keeps retrying. If no auth broker is available, note in the PR that this was not tested.
- [x] 1.3 Tick AUDIT.md item 1 with the commit.

## 2. Night-mode window (AUDIT item 2)

- [x] 2.1 In `main/LVGL_UI/weather_station_ui.c` (the `is_night` line in `weather_station_ui_update()`), use `START <= END ? (h >= START && h < END) : (h >= START || h < END)`. Verify with a host one-liner that goes through all 24 hours for (22, 8), (1, 6) and (8, 8), giving 10, 5 and 0 night hours.
- [x] 2.2 Update the night-mode comment in `config/app_config.h.example` to say any start and end work, including windows that don't cross midnight, and that equal values disable night mode. Verify the example still builds when copied to `app_config.h`.
- [x] 2.3 Tick AUDIT.md item 2.

## 3. Button responsiveness and race (AUDIT items 3, 4)

- [x] 3.1 In `main/button_handler.c` `button_task()`, wrap the `weather_station_toggle_rgb()` and `weather_station_cycle_backlight()` calls in `lvgl_port_lock(0)` / `lvgl_port_unlock()`, and include `esp_lvgl_port.h`. Verify: the build is clean, and 20 fast presses leave the backlight at the expected level with no hang. _(Verified on hardware 2026-09-25.)_
- [x] 3.2 In `main/main.c`, move `button_handler_init()` to just after `BK_Light(5)`, before `wifi_connect_init()`. Verify: boot with the AP off, and a short press changes the backlight within 1 s of it turning on (the boot log shows `BUTTON` lines before the WiFi timeout). _(Boot log: button initialized at 760 ms, before WiFi. AP-off press test passed.)_
- [x] 3.3 Update the AGENTS.md Button State Machine note to say button actions run under the LVGL lock. Tick AUDIT.md items 3 and 4. _(Verified on hardware 2026-09-25.)_

## 4. Backlight duty accuracy (AUDIT item 5)

- [x] 4.1 In `main/LCD_Driver/ST7789.c` `BK_Light()`, replace the duty formula with `LEDC_MAX_Duty * Light / 100` and drop the `Light == 0` special case. Verify with a host one-liner that 0, 1, 5, 10, 25 and 100 map to 0, 81, 409, 819, 2047 and 8191.
- [x] 4.2 Flash, and at night (or in a dark room) check that the 1% level is still readable. If it isn't, change the night level and the "1%" entry to 2 in `weather_station_ui.c`, `README.md` (Button Controls, Automatic Night Mode) and `AGENTS.md` (Backlight Control). Record the outcome in the PR.
- [x] 4.3 Tick AUDIT.md item 5 with the chosen outcome.

## 5. Panel power command (AUDIT item 6)

- [x] 5.1 Take a reference photo of the display. Then, in `Vernon_ST7789T.c`, change the `0xD0` length from 1 to 2, flash, and compare contrast, colour and flicker. Keep the change if the image is the same or better; otherwise revert and note "vendor value intentionally truncated" in AUDIT.md. _(Verified on hardware 2026-09-25.)_
- [x] 5.2 Tick AUDIT.md item 6 with the outcome.

## 6. Integration check

- [x] 6.1 Run the AGENTS.md Testing Checklist on hardware: WiFi, MQTT data, display values, short and long press, and night mode (temporarily set the night hours around the current hour). Verify all items pass, and that the boot log shows no new warnings.
