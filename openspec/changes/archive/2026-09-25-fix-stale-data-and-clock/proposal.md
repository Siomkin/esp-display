# Proposal

## Why

The second audit round (`AUDIT.md` items 18–21) found that the screen can present wrong information as if it were current:
- **Stale values:** sensor readings stay on screen indefinitely after their source stops.
- **Frozen clock:** the clock stops at the last minute it received.
- **Made-up trend:** the trend keeps plotting a copy of the last value, drawing a flat line of fake data.
- **UTC clock:** before a date arrives, the clock shows UTC instead of local time.
- **Night flash:** entering the auto level at night flashes the backlight to 100%.
- **Silent failures:** some start-up failures leave a feature dead with no log line.

## What Changes

- **Stale sensors:** a sensor value with no update for 5 minutes is shown as `--` and leaves a gap in the trend instead of a repeated value (item 18).
- **Counting clock:** the clock keeps counting on the device's own timer between time messages. It shows `--:--` only after 1 hour with no time message (item 18).
- **Local time before a date:** before a date has arrived, the clock shows local time (the timezone offset applied to the hour), not raw UTC (item 19).
- **No night flash:** selecting the auto level at night goes straight to 1%, with no 100% flash (item 20).
- **No silent start-up failures:** a failure in button, backlight, RGB or MQTT start-up is reported and stops the firmware, matching the display and SPI start-up (item 21). No spec: this is an internal robustness rule, verified by reading the code.
- **Illuminance placeholder:** it now reads `-- lx` before data arrives, matching the other placeholders, instead of `0 lx`, which looked like a real reading.

## Capabilities

### New Capabilities
- `sensor-display`: how sensor values and the outside-temperature trend are shown, including placeholders and staleness.
- `clock-display`: how the time and date are derived from MQTT, the timezone offset, counting between updates, and staleness.

### Modified Capabilities
- `backlight-control`: the Night-mode window requirement gains the rule that entering the auto level applies the night level immediately.

## Impact

- `main/Wireless/mqtt_handler.c` and `.h`: a last-update tick per value, and staleness applied in `mqtt_get_sensor_data()`.
- `main/LVGL_UI/weather_station_ui.c`: counting clock, time fallback with offset, `--` placeholders, trend gaps, and immediate auto brightness.
- `main/button_handler.c`, `main/LCD_Driver/ST7789.c`, `main/RGB/RGB.c`, `main/main.c`: error checks.
- Docs: README (display section and troubleshooting), AGENTS.md (MQTT data flow).
- No config-file changes: the timeouts are constants, not per-device settings.
