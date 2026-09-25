# Design

## Context

This change touches nine modules, so this document fixes the order and the shape of the two non-trivial refactors: the MQTT topic table and the brightness table. There's no unit-test harness, so "no behaviour change" is checked by a clean build plus the AGENTS.md hardware checklist.

## Goals / Non-Goals

**Goals:**
- Every step leaves the tree building with no new warnings, so each can be its own commit and bisected.
- Adding a numeric MQTT sensor touches the config, the table and the UI, and nothing else.

**Non-Goals:**
- Changing brightness levels, colours, timing, log text, or init order.
- Removing the boot WiFi wait. `WIFI_CONNECTED_BIT` and the event group stay because `wifi_connect_init()` still uses them. Only the unused getters and `WIFI_FAIL_BIT` go.
- Driver replacement (AUDIT item 11).

## Decisions

1. **Topic table** (item 7). A static array of `{const char *topic, float *value, bool *valid, const char *log_fmt}` covers the four float topics, pointing into `sensor_data`. `MQTT_EVENT_CONNECTED` loops over it to subscribe; `MQTT_EVENT_DATA` loops to match, then falls through to two explicit `strcmp`s for date and time, which are copied as strings. Rejected: putting date and time in the table with a type tag. Two special cases are clearer than a discriminated union for two rows.
2. **Brightness table** (item 8). `static const uint8_t levels[] = {0, 1, 5, 10, 25, 100};` with `AUTO_LEVEL = 5` and `BOOT_LEVEL = 2`. A small public `weather_station_backlight_on()` applies `levels[BOOT_LEVEL]`, and `main.c` calls it in place of `BK_Light(5)`, so the boot brightness lives only in the UI module. Night mode uses `BK_Light(is_night ? 1 : levels[AUTO_LEVEL])`. This keeps the night-window condition and the locking from `fix-audit-behavior` unchanged.
3. **RGB ramp** (item 9). For step `i` in 0..191: `seg = i / 64`, `k = i % 64`, `a = 64 - k`, `b = k + 1`. Segment 0 gives (a, b, 0), segment 1 gives (0, a, b), and segment 2 gives (b, 0, a), each scaled ×3 when sent. A host check already confirmed that this matches all 192 rows of the current table; task 3.1 re-runs it before the table is deleted.
4. **RGB state** (item 10). `RGB.c` gains `void RGB_Toggle(void)`, which flips `rgb_enabled`, notifies the task and logs. The button calls it directly, under the LVGL lock added by `fix-audit-behavior`. The lock is harmless here and keeps the button task uniform.
5. **Includes** (item 17). `ST7789.h` drops `lvgl.h` and `LVGL_Driver.h`. `main.c` and `SD_SPI.c` include `LVGL_Driver.h` themselves, and `weather_station_ui.h` drops `mqtt_client.h`. The build is the test.

## Risks / Trade-offs

- [The computed RGB ramp differs from the table] → Compare all 192 entries on the host (task 3.1) before deleting the table.
- [An include removal breaks a translation unit that relied on the transitive include] → Do the include cleanup last and in its own commit, and fix every file the compiler reports.
- [The removed public functions are used by out-of-tree code] → There are none: this is a single-app firmware with no exported API, and grep shows no callers in the repo, docs or skills.
