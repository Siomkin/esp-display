# Tasks

Build with `docker run --rm -v "$PWD":/project -w /project espressif/idf:v6.1 idf.py build` ("builds clean" means no warnings from `main/`), and flash with `.claude/skills/flash-device/scripts/flash.sh --monitor 30`. For short stale tests, temporarily set `SENSOR_STALE_MS` to 20 s and `CLOCK_STALE_MS` to 60 s, and restore them before committing.

## 1. Staleness in the data layer (AUDIT item 18)

- [ ] 1.1 Add a `TickType_t` last-update field per value to `sensor_data_t` and a `tick` pointer to `float_topics[]`. Set it wherever `_valid = true` is set. Verify: builds clean.
- [ ] 1.2 In `mqtt_get_sensor_data()`, clear `_valid` in the returned copy when a field is older than `SENSOR_STALE_MS` (5 min, floats) or `CLOCK_STALE_MS` (60 min, time and date). Verify: with short timeouts, unplug the router (or stop the broker) and all four sensor fields show placeholders once the timeout passes, then recover within 1 s of data returning.

## 2. Display (AUDIT items 18, 19)

- [ ] 2.1 In `weather_station_ui_update()`, write the placeholder text for invalid float fields, and change illuminance's initial text to `-- lx`. Verify: boot with the broker unreachable, and the screen shows `--°C`, `--°C`, `Hum: --%` and `-- lx`.
- [ ] 2.2 Add the counting clock (design decision 2) and the offset fallback without a date (decision 3), and show `--:--` when the time is stale (decision 4). Verify with a host check of the time arithmetic (offset 3: 22:30 → 01:30; 18:33 + 600 s → 21:43 local), and on the board by blocking the time topic for 10 minutes while the clock keeps advancing.
- [ ] 2.3 Check that the trend has a gap while the outside reading is stale (with the short timeout, stop the outside sensor or broker for 3 minutes). Verify on screen: the line breaks and resumes.
- [ ] 2.4 Update README (Display Layout, troubleshooting "stale or `--` values") and the AGENTS.md MQTT Data Flow note on staleness. Verify: the docs name only real symbols and timeouts.

## 3. Auto brightness (AUDIT item 20)

- [ ] 3.1 Extract `apply_auto_brightness()` and call it when entering the auto level (design decision 5). Verify: temporarily set the night hours around the current hour and press from 25% to auto; the backlight goes straight to 1% with no flash.

## 4. Error checks (AUDIT item 21)

- [ ] 4.1 Wrap the unchecked calls listed in AUDIT item 21 in `ESP_ERROR_CHECK`. Make `RGB_Start()` return `esp_err_t` and check `pdPASS`, and check the results of `button_handler_init()`, `RGB_Start()` and `mqtt_client_init()` in `main.c`. Verify: builds clean, and the boot log is unchanged on a healthy board.

## 5. Integration

- [ ] 5.1 Restore the real timeouts, flash both boards, and run the AGENTS.md Testing Checklist plus the three new specs' scenarios that can be triggered by hand. Tick AUDIT items 18–21 with the commit. Verify: every checklist item passes.
