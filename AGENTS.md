# AI Agent Context - ESP32-C6 Weather Station

This document provides context for AI assistants working on this project.

## Project Overview

**Type**: ESP32-C6 embedded firmware for weather station display  
**Hardware**: Waveshare ESP32-C6-LCD-1.47 (172×320 ST7789 LCD, WS2812 RGB LED)  
**Framework**: ESP-IDF 6.1  
**UI Library**: LVGL 9.6 via esp_lvgl_port  
**Communication**: WiFi + MQTT  

## Architecture

### Core Components

1. **Display System** (`main/LCD_Driver/`, `main/LVGL_Driver/`, `main/LVGL_UI/`)
   - ST7789 LCD driver with PWM backlight control
   - LVGL integration for UI rendering
   - Weather station UI showing time, date, temperatures, humidity, illuminance
   - Black background with color-coded text elements

2. **Network Layer** (`main/Wireless/`)
   - WiFi connection manager
   - MQTT client subscribing to sensor topics
   - All user settings (WiFi, broker, topics, timezone, night mode) in gitignored `config/app_config.h`

3. **User Input** (`main/button_handler.c/h`)
   - Single BOOT button (GPIO 9) with dual functionality:
     - Short press: Cycle backlight (5%→10%→25%→100% auto→0%→1%→5%)
     - Long press (≥1s): Toggle RGB LED on/off
   - Edge interrupt only wakes the task; task waits 50ms, then samples the settled level
     (ISR-time level reads were unreliable while the contacts bounce and lost presses)

4. **RGB LED** (`main/RGB/`)
   - WS2812 LED on GPIO 8
   - Colorful animation (disabled by default)
   - Can be toggled via long button press

5. **Peripherals** (`main/SD_Card/`)
   - SPI bus init shared with LCD; SD card not mounted (CS held high)

## Key Configuration Files

### Private (Gitignored)
- `config/app_config.h` - The only file to edit per device: WiFi, MQTT broker/login, client ID prefix,
  topics, `TIMEZONE_OFFSET_HOURS`, `NIGHT_MODE_START_HOUR`/`END_HOUR`. Build fails if missing.

### Public (In Git)
- `config/app_config.h.example` - Template for `app_config.h`; add new settings here too
- `main/button_handler.h` - Button GPIO definitions
- `AGENTS.md` - This file (loaded by Claude Code via `CLAUDE.md`)
- `.claude/skills/flash-device/` - Build + flash + boot-log skill

## Important Implementation Details

### Display Layout
```
┌─────────────────────────────────────┐
│ 12:45          (white)      23.5°C  │ (gold, 48pt)
│ MON, 15 JAN    (cyan)       22.1°C  │ (orange, 28pt)
│ ~~~~~~~~ (gold trend line)          │ outside temp, 1 sample/min, last 2h
│ Hum: 57%       (white)      1234 lx │ (white, 32pt)
└─────────────────────────────────────┘
```

**Colors:**
- Time: White
- Date: Cyan (0, 200, 255)
- Outside Temp: Gold (255, 215, 0)
- Inside Temp: Orange (255, 165, 0)
- Humidity/Illuminance: White

### Backlight Control
- Default: 5% on boot
- Levels: 0%, 1%, 5%, 10%, 25%, 100%
- Auto night mode (default 22:00-08:00, `NIGHT_MODE_START_HOUR`/`END_HOUR`): 1% brightness, restores previous level after
- Controlled via `BK_Light(uint8_t)` function (0-100)

### Timezone Handling
- MQTT provides UTC time/date
- Offset applied: `TIMEZONE_OFFSET_HOURS * 3600` (default: 3 for Minsk UTC+3)
- Configured in `config/app_config.h`

### MQTT Data Flow
1. Subscribe to topics on connection
2. Parse incoming messages in `mqtt_event_handler()`
3. Store in `sensor_data_t` structure
4. UI updates every 1 second via `weather_station_ui_update()`

### Button State Machine
```
Edge → ISR (task notify) → Task: wait 50ms, sample level → state changed? → on release: measure duration → Action
                                ├─ <1s: Cycle backlight
                                └─ ≥1s: Toggle RGB
```

## Code Style & Patterns

### Naming Conventions
- Functions: `snake_case` (e.g., `weather_station_ui_init()`)
- Defines: `UPPER_SNAKE_CASE` (e.g., `MQTT_TOPIC_TEMP_OUTSIDE`)
- Static variables: `snake_case` with `static` keyword
- File-local state: Static variables at file scope

### Error Handling
- Use `ESP_ERROR_CHECK()` for critical operations
- Log with `ESP_LOGI()`, `ESP_LOGW()`, `ESP_LOGE()`
- Return `esp_err_t` for functions that can fail

### FreeRTOS Usage
- Tasks for background operations (RGB animation, button handling)
- Task notifications for ISR → task and RGB on/off wakeups
- `vTaskDelay()` for timing
- Priority: Button task = 10, RGB task = 4

## Build System

### CMake Structure
- Root `CMakeLists.txt` defines project
- `main/CMakeLists.txt` lists all source files
- Dependencies fetched by the component manager into `managed_components/` (gitignored)

### Dependencies
- `lvgl/lvgl` (9.6) - UI library
- `espressif/esp_lvgl_port` (2.9) - LVGL task/tick/flush; call LVGL from other tasks only under `lvgl_port_lock()`
- `espressif/led_strip` (3.0.3) - WS2812 control
- `espressif/mqtt` (1.1) - MQTT client (no longer built into ESP-IDF 6.x)
- ESP-IDF built-in: wifi, nvs_flash, etc.

### Build Commands
No local ESP-IDF 6.x on the dev Mac (the 5.5.2 install is too old) → use the `flash-device` skill, which builds in Docker:
```bash
.claude/skills/flash-device/scripts/flash.sh --monitor 30   # build + flash + 30s boot log
docker run --rm -v "$PWD":/project -w /project espressif/idf:v6.1 idf.py build   # build only
```
With a local ESP-IDF 6.x (never run interactive `idf.py monitor` from an agent):
```bash
source ~/esp/v6.1/esp-idf/export.sh
idf.py build
idf.py -p /dev/cu.usbmodem21201 flash
```

## Common Modifications

### Adding New MQTT Topic
1. Add define to `config/app_config.h` and `config/app_config.h.example`: `#define MQTT_TOPIC_NEW "/path/to/topic"`
2. Subscribe in `mqtt_handler.c`: `esp_mqtt_client_subscribe()`
3. Parse in `mqtt_event_handler()`: Add `else if` case
4. Add field to `sensor_data_t` structure
5. Update UI in `weather_station_ui.c`

### Changing Button Behavior
1. Modify `button_handler.c`: Update `button_task()` logic
2. Update `button_handler.h`: Update documentation comment
3. Update `README.md` → "Button Controls" section: Document new behavior

### Adjusting Display Colors
Edit `weather_station_ui.c`:
```c
lv_obj_set_style_text_color(label, lv_color_make(R, G, B), 0);
```

### Changing Brightness Levels
Edit `weather_station_ui.c` → `weather_station_cycle_backlight()`:
- Modify `switch` cases
- Update modulo value: `backlight_level = (backlight_level + 1) % N;`

## Hardware Pinout

| Function | GPIO | Notes |
|----------|------|-------|
| LCD MOSI | 6 | SPI |
| LCD SCLK | 7 | SPI |
| LCD CS | 14 | |
| LCD DC | 15 | |
| LCD RST | 21 | |
| LCD BL | 22 | PWM backlight |
| RGB LED | 8 | WS2812 |
| BOOT Button | 9 | Active-low, internal pull-up |
| SD MISO | 5 | SPI |
| SD CS | 4 | |

## Testing Checklist

When making changes, verify:
- [ ] Code compiles without warnings
- [ ] WiFi connects successfully
- [ ] MQTT receives data from all topics
- [ ] Display shows correct data
- [ ] Button short press cycles backlight
- [ ] Button long press toggles RGB
- [ ] Night mode activates at `NIGHT_MODE_START_HOUR`
- [ ] Timezone offset is correct
- [ ] No memory leaks (check heap)
- [ ] No sensitive data in committed files

## Git Workflow

### Gitignored Files
- `config/app_config.h` - Contains passwords and device settings
- `build/`, `managed_components/` - Build artifacts, downloaded components
- `sdkconfig` - Generated from `sdkconfig.defaults`; put tracked config changes in `sdkconfig.defaults`
- `.claude/settings.local.json` - Per-user Claude Code settings
- `.DS_Store` - Mac system files

### Safe to Commit
- All `.h` files except `config/app_config.h`
- All `.c` files
- `config/app_config.h.example` - Placeholders only
- Documentation: `*.md` (including `AGENTS.md`, `CLAUDE.md`)
- `CMakeLists.txt`, `dependencies.lock`, `sdkconfig.defaults`, `partitions.csv`

## Debugging Tips

### Serial Monitor
- Baud rate: 115200
- Look for tags: `MAIN`, `WiFi`, `MQTT`, `WeatherUI`, `BUTTON`, `WS_LVGL`

### Common Issues
1. **WiFi won't connect**: Check SSID/password in `config/app_config.h`
2. **MQTT no data**: Verify broker URI and topics in `config/app_config.h`
3. **Display blank**: Check backlight level (press button)
4. **Button not working**: Verify GPIO 9, check debounce timing
5. **RGB stuck on**: Long press to toggle, or check `is_rgb_enabled` default

### Memory Monitoring
```c
ESP_LOGI(TAG, "Free heap: %" PRIu32, esp_get_free_heap_size());  // uint32_t: %d warns on RISC-V
```

## Performance Considerations

- LVGL refresh: 30ms period in esp_lvgl_port task; labels redraw only when text changes
- UI update: 1 second
- Button debounce: 50ms
- RGB animation: 20ms per frame
- MQTT: Event-driven (no polling)

## Future Enhancement Ideas

- Touch screen support
- Weather forecast display
- Historical data graphs
- Multiple timezone support
- OTA firmware updates
- Web configuration interface
- Battery level monitoring
- Sleep mode for power saving

## Contact & Resources

- **ESP-IDF Docs**: https://docs.espressif.com/projects/esp-idf/
- **LVGL Docs**: https://docs.lvgl.io/9.6/
- **Board Wiki**: https://www.waveshare.com/wiki/ESP32-C6-LCD-1.47
- **MQTT Protocol**: https://mqtt.org/
