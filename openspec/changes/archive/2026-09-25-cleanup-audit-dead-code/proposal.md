# Proposal

## Why

The code audit (`AUDIT.md` items 7–10 and 12–17) found about 130 lines that can be deleted or collapsed without changing behaviour: unused APIs, leftovers from the vendor example, redundant state, a hand-written colour table, and a five-place recipe for adding an MQTT topic. Removing them makes the next behaviour change cheaper and stops readers from trusting dead code (for example, the "Sleep Out" comment that labels the display-inversion command).

## What Changes

- An MQTT topic table replaces the subscribe list and the `strcmp` chain for the four numeric sensors (item 7).
- A lookup table replaces the brightness `switch`. The redundant `auto_brightness_enabled` and `saved_brightness` are removed, and the boot brightness is defined in one place (item 8).
- The RGB rainbow is computed instead of read from a 192-row table (item 9).
- The duplicate RGB on/off state and the `weather_station_toggle_rgb()` middle man are removed (item 10).
- Unused functions, macros, commented-out code, the unused fade ISR install and misleading comments are deleted (items 12–16).
- The circular `ST7789.h` ↔ `LVGL_Driver.h` include is broken and unused includes are dropped (item 17).

No behaviour changes: the display, button, RGB animation, MQTT and WiFi act exactly as before.

**Depends on** `fix-audit-behavior`. Items 7, 8 and 10 edit the same functions as its items 1–3, so apply it after that change is merged.

Out of scope: replacing the vendored ST7789T driver (AUDIT item 11, separate change).

## Capabilities

### New Capabilities
None.

### Modified Capabilities
None. This is a pure refactor with no spec-level behaviour change, so `skip_specs: true` is set.

## Impact

- `main/Wireless/mqtt_handler.c/.h`, `main/Wireless/wifi_connect.c/.h`
- `main/LVGL_UI/weather_station_ui.c/.h`, `main/button_handler.c`, `main/main.c`
- `main/RGB/RGB.c/.h`
- `main/LCD_Driver/ST7789.c/.h`, `main/LCD_Driver/Vernon_ST7789T/Vernon_ST7789T.c`
- `main/SD_Card/SD_SPI.c/.h`, `main/LVGL_Driver/LVGL_Driver.h`
- Docs: AGENTS.md ("Adding New MQTT Topic", "Changing Brightness Levels", "RGB stuck on" tip).
- Public function removals (no callers anywhere): `weather_station_toggle_display`, `weather_station_toggle_rgb`, `wifi_is_connected`, `wifi_get_event_group`, `mqtt_is_connected`, the global `Flash_Size`.
