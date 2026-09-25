# Design

## Context

`sensor_data_t` (`mqtt_handler.h`) holds a value and a `_valid` flag per field. The MQTT task writes it under `sensor_lock`, and the UI copies it once a second through `mqtt_get_sensor_data()`. Nothing records *when* a field was written. The controller publishes `HH:MM` roughly once a minute, and the date as `YYYY-MM-DD` (retained in the current setup). The button task calls `weather_station_cycle_backlight()` under the LVGL lock, and the UI update also runs under that lock (see the archived `fix-audit-behavior`).

## Goals / Non-Goals

**Goals:**
- Staleness is decided in one place (`mqtt_get_sensor_data()`), so the UI and the trend keep using `_valid` unchanged.
- No new tasks, timers or config settings.

**Non-Goals:**
- SNTP or any other time source. MQTT stays the only clock source.
- Showing staleness any other way (icons, colours).
- Per-device timeouts in `app_config.h`.

## Decisions

1. **Last-update tick per field.** Add `TickType_t` fields (`temp_outside_tick`, ..., `time_tick`, `date_tick`) to `sensor_data_t`, set with `xTaskGetTickCount()` wherever the matching `_valid = true` is set. The `float_topics[]` row gains a `TickType_t *tick` pointer. `mqtt_get_sensor_data()` clears `_valid` in the *copy* it returns when `now - tick > SENSOR_STALE_MS` (5 min) for the four floats, or `> CLOCK_STALE_MS` (60 min) for time and date. Tick arithmetic is unsigned, so wrap-around is safe; the tick counter wraps after about 49 days at 1 kHz. Rejected: a periodic sweep that clears the stored flags. It needs a timer and races with the MQTT writer.
2. **Counting clock.** The UI already converts date+time to a `time_t` each second. It adds `(now_tick - time_tick) / configTICK_RATE_HZ` seconds before applying the offset, and keeps the minute boundary. A new message resets `time_tick`, which corrects any drift. `date_tick` is only used for staleness. Rejected: `settimeofday()` plus `time()`, because it makes the UI depend on a global clock that the WiFi stack or SNTP could later change without warning.
3. **Time fallback** (no valid date): parse `HH:MM`, then show `(hh + TIMEZONE_OFFSET_HOURS + 24) % 24` with the same counting. Leave the date label unchanged (Extract Function `local_hour()`). The night-mode check uses this path too, so night mode works before a date arrives.
4. **Stale clock:** when time is invalid in the returned copy, the UI sets the time label to `--:--` and leaves the date. Night mode keeps its last state rather than guessing.
5. **Auto level applies immediately.** Extract `apply_auto_brightness(int local_hour)` from the night-mode block. The UI update stores the last known local hour (`-1` if unknown). `weather_station_cycle_backlight()` calls `apply_auto_brightness(last_local_hour)` on entering the auto level; with `-1` it falls back to 100%. Both callers run under the LVGL lock, so `last_local_hour` needs no extra locking.
6. **Placeholders:** the update writes the same text as `weather_station_ui_init()` when a field is invalid. Illuminance's initial text changes from `0 lx` to `-- lx`. The trend sampler already writes `LV_CHART_POINT_NONE` for invalid readings, so decision 1 gives it gaps automatically.
7. **Error checks** (item 21): `ESP_ERROR_CHECK` on the listed calls. `RGB_Start()` and `mqtt_client_init()` return `esp_err_t`, and `main.c` wraps them together with `button_handler_init()` in `ESP_ERROR_CHECK`. A failure therefore aborts and resets, the same as a failed `LCD_Init()` today.

## Risks / Trade-offs

- [A sensor that publishes only on change (rarely) goes to `--` every 5 minutes] → Every current topic publishes at least once a minute (the boot log shows 37 messages in 30 s). If a slow sensor is added later, give its `float_topics[]` row its own timeout.
- [The counting clock drifts between messages] → The ESP32 crystal is accurate to within seconds per day, and every time message resets it.
- [Error checks turn a previously silent partial failure into a reboot loop] → That's intended and matches the display path. The boot log then names the failing call instead of the device looking half-alive.
