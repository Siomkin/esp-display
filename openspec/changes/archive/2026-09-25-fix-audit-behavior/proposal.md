# Proposal

## Why

The code audit (`AUDIT.md` items 1–6) found places where the firmware does something different from what the config template, README and code comments promise. The MQTT login is silently ignored. Night mode breaks for any window that doesn't cross midnight. The backlight "1%" is really 2.1%. A button press can be undone by the UI task. The button doesn't work for up to 30 s at boot while WiFi is down. One panel init command is truncated. Each one misleads whoever configures or debugs the device.

## What Changes

- The MQTT client sends the username and password from `config/app_config.h` when they are set, and connects anonymously when they are empty (item 1).
- The night-mode window works for any start and end hour, whether or not it crosses midnight (item 2).
- A button press that changes the backlight can no longer be overridden by a concurrent UI update (item 3).
- The BOOT button works from the moment the UI is shown, instead of after the WiFi connect wait (item 4).
- Backlight PWM duty matches the named percentage (1% ≈ 1% duty, not 2.1%). **BREAKING (visual)**: levels 1%, 5% and 10% get dimmer, so check at night on hardware (item 5).
- The ST7789T Power Control 1 command sends both of its parameter bytes (item 6). No spec: panel register values are not observable behaviour, so this is verified on hardware only.

Out of scope: the P3 cleanup (change `cleanup-audit-dead-code`) and swapping the panel driver for the ESP-IDF built-in one (AUDIT item 11, a separate future change).

## Capabilities

### New Capabilities
- `mqtt-connection`: how the device authenticates to the MQTT broker.
- `backlight-control`: brightness levels, their accuracy, and the automatic night-mode window.
- `button-controls`: BOOT button availability and how its actions interact with background updates.

### Modified Capabilities
None. There are no existing specs.

## Impact

- `main/Wireless/mqtt_handler.c`: MQTT client config.
- `main/LVGL_UI/weather_station_ui.c`: night-mode condition.
- `main/button_handler.c`: LVGL lock around button actions.
- `main/main.c`: init order.
- `main/LCD_Driver/ST7789.c`: `BK_Light()` duty formula.
- `main/LCD_Driver/Vernon_ST7789T/Vernon_ST7789T.c`: `0xD0` parameter length.
- Docs: `README.md` (night mode and button sections), `AGENTS.md` (Backlight Control), `config/app_config.h.example` (night-mode comment).
- No new dependencies, and no config-file format change: existing `app_config.h` files keep working.
