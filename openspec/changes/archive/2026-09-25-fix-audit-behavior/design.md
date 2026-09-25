# Design

## Context

The six fixes are small and touch six files. A design doc is warranted for two reasons: two of them change what the user sees on hardware (backlight duty, and panel power settings), and one adds locking across tasks. Timing facts that shape the approach:

- The button task runs at priority 10 and the LVGL task (`esp_lvgl_port`) at 4. The ESP32-C6 has a single core, so the button task can preempt the UI update mid-way. The reverse can't happen.
- `lvgl_port_lock()` is already the project's rule for touching UI state from another task (AGENTS.md, esp_lvgl_port).
- `wifi_connect_init()` blocks for up to `WIFI_CONNECT_TIMEOUT_MS` (30 s). MQTT and the UI don't need it to finish.

## Goals / Non-Goals

**Goals:**
- Meet the specs in `specs/` with the smallest diff in each file.
- Keep existing `config/app_config.h` files valid without edits.

**Non-Goals:**
- Refactoring the backlight `switch` into a table, and any other cleanup (change `cleanup-audit-dead-code`).
- Removing the boot WiFi wait. Moving the button init is enough to meet the spec, and dropping the wait changes boot logging for no user benefit.
- Replacing the vendored ST7789T driver.

## Decisions

1. **MQTT credentials.** Set `.credentials.username` and `.credentials.authentication.password` to the config string, or to `NULL` when it is empty (`MQTT_USERNAME[0] ? MQTT_USERNAME : NULL`). A `NULL` field keeps anonymous brokers working exactly as today. Rejected: an `#if` on the macro value, because the preprocessor can't compare string literals.
2. **Night window.** `START <= END ? (h >= START && h < END) : (h >= START || h < END)`. With `START == END` this gives an empty window (night mode never triggers), which is the least surprising reading.
3. **Race.** The button task takes `lvgl_port_lock(0)` around both `weather_station_cycle_backlight()` and `weather_station_toggle_rgb()`. The UI update already runs with that lock held, from an LVGL timer inside the port task, so no UI-side change is needed. Rejected: a separate mutex for the backlight state. It would duplicate a lock that already exists and add a lock-ordering hazard.
4. **Init order.** Move `button_handler_init()` in `main.c` to just after `BK_Light(5)`, before the WiFi call. The button handlers only touch the UI and RGB, and both are initialised by then.
5. **Duty formula.** `Duty = LEDC_MAX_Duty * Light / 100`, which gives 0 for 0% with no special case. The product fits comfortably in 32 bits. The old `81*(100-Light)` form under-subtracts by 91 steps.
6. **`0xD0` length.** Change the length from 1 to 2 so that `0xA1` is sent. This matches the command's two-parameter definition in the ST7789 datasheet and the comment already in the code.

## Risks / Trade-offs

- [Backlight 1% becomes about half as bright as today; night mode may be too dark to read] → Check on hardware at night. If it's too dim, change the night level and the "1%" cycle entry to `2` in code and docs, not the formula. The spec only requires that the level shown matches the duty.
- [`0xD0` second byte changes the panel's analog supply settings] → Compare the image before and after (contrast, flicker, colour cast). If it's worse, revert that one line and record in AUDIT.md that the vendor value is intentionally truncated.
- [The button task now blocks on the LVGL lock for up to one LVGL refresh (≈30 ms)] → That's well under the 50 ms debounce and invisible to the user.
