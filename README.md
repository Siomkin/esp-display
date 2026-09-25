# ESP32-C6 Weather Station Display

A weather station display for ESP32-C6-LCD-1.47 using LVGL, WiFi, and MQTT to show real-time sensor data.

## Features

- **WiFi Connectivity**: Connects to your local WiFi network with auto-reconnect
- **MQTT Integration**: Subscribes to sensor topics (temperature, humidity, illuminance, time/date)
- **LVGL UI**: Clean interface with color-coded elements displaying:
  - Current time and date (with configurable timezone, default: Minsk UTC+3)
  - Outside and inside temperature
  - Outside temperature trend: line chart, one sample per minute, last 2 hours (resets on reboot)
  - Humidity percentage
  - Illuminance (lux)
- **Button Controls**:
  - Short press: Cycle backlight brightness (5% → 10% → 25% → 100% with auto night mode → 0% → 1% → 5%)
  - Long press (≥1s): Toggle RGB LED on/off
- **Auto Night Mode**: Automatically dims to 1% brightness from 22:00-08:00 (when in 100% auto mode)
- **Real-time Updates**: UI refreshes every second

## Hardware

- **Board**: Waveshare ESP32-C6-LCD-1.47
- **Display**: 1.47" ST7789 LCD (172×320 pixels)
- **RGB LED**: WS2812 on GPIO 8
- **Button**: BOOT button on GPIO 9
- **SD Card**: Slot present, not mounted (SPI bus shared with LCD)
- **Documentation**: [Waveshare Wiki](https://www.waveshare.com/wiki/ESP32-C6-LCD-1.47)

### GPIO Pinout

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

## Quick Start

### Prerequisites

- ESP-IDF >= 6.0 (tested with 6.1)
- USB cable for flashing
- WiFi network (2.4GHz only)
- MQTT broker running and accessible

### 1. Setup ESP-IDF Environment

```bash
source ~/esp/v6.1/esp-idf/export.sh
```

Or build without a local install, using Docker:

```bash
docker run --rm -v "$PWD":/project -w /project espressif/idf:v6.1 idf.py build
```

### 2. Configure the Device

All settings live in one gitignored file, so configuring a board never touches tracked files:

```bash
cp config/app_config.h.example config/app_config.h
```

Edit `config/app_config.h`: WiFi SSID/password, MQTT broker URI and login, client ID prefix
(a MAC suffix is appended per board), sensor topics, `TIMEZONE_OFFSET_HOURS`, and night-mode hours.
The build stops with a clear message if the file is missing.

### 3. Build and Flash

This board uses the ESP32-C6 **native USB Serial/JTAG** (no external USB-UART chip). On macOS the port is typically `/dev/cu.usbmodem*`.

**Option A: Local ESP-IDF 6.x**
```bash
idf.py build
idf.py -p /dev/cu.usbmodem21201 flash monitor
```

**Option B: No local ESP-IDF (Docker build + esptool flash)**
```bash
.claude/skills/flash-device/scripts/flash.sh --monitor 30
```

**Option C: Flash via USB-JTAG (OpenOCD)** — use when `idf.py flash` fails with `No serial data received` (USB still enumerates, but the serial ACM channel is wedged). BOOT button is not required; JTAG still works in that state:

```bash
idf.py build

openocd -f board/esp32c6-builtin.cfg \
  -c "program_esp build/bootloader/bootloader.bin 0x0 verify" \
  -c "program_esp build/partition_table/partition-table.bin 0x8000 verify" \
  -c "program_esp build/ESP32-C6-LCD-1.47-Test.bin 0x10000 verify reset exit"
```

`OPENOCD_SCRIPTS` must be set (ESP-IDF `export.sh` does this). Unplug/replug USB often restores normal serial flashing afterward.

Replace `/dev/cu.usbmodem21201` with your actual port:
- **macOS**: `/dev/cu.usbmodem*` (native USB Serial/JTAG)
- **Linux**: `/dev/ttyACM*`
- **Windows**: `COM3`, `COM4`, etc.

### 4. Expected Output

```
I (123) MAIN: Initializing hardware...
I (456) WiFi: WiFi started, connecting...
I (789) WiFi: Got IP:192.168.1.xxx
I (890) WiFi: Connected to AP SSID:YourWiFiSSID
I (912) MAIN: Starting MQTT client...
I (1234) MQTT: MQTT_EVENT_CONNECTED
I (1235) MQTT: Subscribed to sensor topics
I (1456) WeatherUI: Weather station UI initialized
I (2000) MQTT: Outside temperature: 23.5°C
```

## Configuration

All user settings are in `config/app_config.h` (gitignored), created from the tracked
template `config/app_config.h.example`. Nothing else needs editing to set up a board.

```
config/
├── app_config.h.example         # Template (in git)
└── app_config.h                 # Your settings: WiFi, MQTT, topics, timezone, night mode (gitignored)
```

### Data Format

| Data Type | Unit | Format | Example |
|-----------|------|--------|---------|
| Temperature | Celsius (°C) | Float, 1 decimal | `23.5°C` |
| Humidity | Percentage (%) | Integer | `Hum: 57%` |
| Illuminance | Lux (lx) | Integer | `1234 lx` |
| Time | 24-hour | HH:MM | `12:45` |
| Date | Uppercase | DAY, DD MON | `MON, 15 JAN` |

### Update Frequency

- **UI Refresh**: 1 second
- **LVGL**: esp_lvgl_port task, 30ms refresh period; labels redraw only when their text changes
- **MQTT**: Real-time (event-driven)
- **Button Debounce**: 50ms

## Button Controls

### BOOT Button (GPIO 9)

#### Short Press (< 1 second)
Cycles through backlight brightness levels:
1. **5%** (default on boot) - Fixed brightness
2. **10%** - Fixed
3. **25%** - Fixed
4. **100%** - Auto mode with night mode
5. **0%** - Display off
6. **1%** - Very dim, fixed
7. Cycles back to 5%

#### Long Press (≥ 1 second)
Toggles RGB LED on/off (default: off on boot)

### Automatic Night Mode

When backlight is set to **100% (auto mode)**:
- **Night (22:00-08:00)**: Automatically dims to 1%
- **Day (08:00-22:00)**: Restores previous brightness

At other brightness levels (0%, 1%, 5%, 10%, 25%), brightness stays fixed.

### Implementation Details

- **Debounce Time**: 50ms
- **Long Press Threshold**: 1000ms
- **Button Type**: Active-low with internal pull-up
- **Interrupt-driven**: Efficient handling on press/release
- **Debouncing**: edge interrupt wakes a task that samples the settled level after 50ms

## Display Layout

```
┌─────────────────────────────────────┐
│ 12:45          (white)      23.5°C  │ (gold, 48pt)
│ MON, 15 JAN    (cyan)       22.1°C  │ (orange, 28pt)
│                                     │
│                                     │
│ Hum: 57%       (white)      1234 lx │ (white, 32pt)
└─────────────────────────────────────┘
```

### Display Elements

- **Top Left**: Time (HH:MM, white, 48pt) and Date (DAY, DD MON, cyan, 20pt)
- **Top Right**: Outside Temperature (gold, 48pt) and Inside Temperature (orange, 28pt)
- **Bottom Left**: Humidity percentage (white, 20pt)
- **Bottom Right**: Illuminance in lux (white, 32pt)
- **Background**: Black

### Color Scheme

| Element | Color | RGB Values |
|---------|-------|------------|
| Time | White | 255, 255, 255 |
| Date | Cyan | 0, 200, 255 |
| Outside Temp | Gold | 255, 215, 0 |
| Inside Temp | Orange | 255, 165, 0 |
| Humidity | White | 255, 255, 255 |
| Illuminance | White | 255, 255, 255 |
| Background | Black | 0, 0, 0 |

## Project Structure

```
main/
├── button_handler.c/h   # Button control (backlight, RGB)
├── LCD_Driver/          # ST7789 display driver
├── LVGL_Driver/         # LVGL integration
├── LVGL_UI/             # Weather station UI
├── RGB/                 # RGB LED control (WS2812)
├── SD_Card/             # SPI bus init (shared with LCD); SD card not mounted
├── Wireless/
│   ├── wifi_connect.c/h # WiFi connection manager
│   └── mqtt_handler.c/h # MQTT client implementation
└── main.c               # Main application entry point
```

## Dependencies

Managed via `main/idf_component.yml`:
- **ESP-IDF**: >= 6.0
- **LVGL**: ~9.6.0 (UI library)
- **esp_lvgl_port**: ^2.9.0 (LVGL task, tick, display flush)
- **led_strip**: ^3.0.3 (WS2812 control)
- **mqtt**: ^1.1.0 (moved out of ESP-IDF in v6.0)

Built-in ESP-IDF components:
- wifi
- nvs_flash
- esp_timer
- driver (GPIO, SPI, LEDC)

## Troubleshooting

### WiFi Connection Issues

**Symptoms**: WiFi won't connect, timeout errors, or display stops updating after router reboot

**Solutions**:
- Verify WiFi settings in `config/app_config.h`
- Ensure network is 2.4GHz (ESP32 doesn't support 5GHz)
- Check network is in range
- Monitor serial output for connection status (`[WiFi]` tag) — reconnect retries forever with backoff
- After a router reboot or AP firmware update, data should resume within ~10–30s once the AP is back
- Verify SSID and password are correct

### MQTT Not Receiving Data

**Symptoms**: No sensor data on display, MQTT connection fails

**Solutions**:
- Verify broker URI in `config/app_config.h`
- Check topic names in `config/app_config.h` match your setup
- Ensure broker is accessible from ESP32 network
- Confirm WiFi is connected (`Got IP` in serial) — MQTT auto-reconnects after WiFi recovers
- Test broker with: `mosquitto_sub -h BROKER_IP -t '#' -v`
- Check MQTT logs in serial output with tag `[MQTT]`

### Display Issues

**Symptoms**: Display blank, no backlight, wrong colors

**Solutions**:
- Check backlight level (press BOOT button to cycle)
- Verify ST7789 connections (see GPIO pinout)
- Ensure LVGL is properly initialized
- Check serial output for initialization errors
- Verify power supply is adequate (USB should be sufficient)

### Button Not Working

**Symptoms**: Button presses don't change backlight or RGB

**Solutions**:
- Verify GPIO 9 connection
- Check debounce timing (50ms)
- Monitor serial output for button events with tag `[BUTTON]`
- Ensure button task is running (priority 10)

### RGB LED Issues

**Symptoms**: RGB LED doesn't turn on or shows wrong colors

**Solutions**:
- Long press button to toggle RGB on
- Verify WS2812 connection to GPIO 8
- Check power supply (WS2812 can draw significant current)
- Ensure `rgb_enabled` in `main/RGB/RGB.c` is toggled (long press)

### Flash / Serial Connection Issues

**Symptoms**: `Failed to connect to ESP32-C6: No serial data received` while the board still appears as Espressif `USB JTAG/serial debug unit`

**Cause**: The USB serial (ACM) channel can stop responding while USB-JTAG still works. Holding BOOT is not required for this board’s native USB.

**Solutions**:
- Unplug and replug the USB cable, then retry `idf.py -p PORT flash`
- Flash over USB-JTAG instead (see Option C in Quick Start)
- Confirm nothing else has the port open (`lsof /dev/cu.usbmodem*`)
- Prefer the `cu.*` device on macOS (not `tty.*`)

### Build Errors

**Symptoms**: Compilation fails, missing dependencies

**Solutions**:
- Source ESP-IDF environment: `source ~/esp/v6.1/esp-idf/export.sh`
- Verify target is set to esp32c6: `idf.py set-target esp32c6`
- Clean build: `idf.py fullclean && idf.py build`
- Check all files are in correct locations
- If the project path moved, update local paths in `dependencies.lock` or regenerate it
- Verify `dependencies.lock` is present

### Memory Issues

**Symptoms**: Heap allocation failures, crashes

**Solutions**:
- Monitor free heap: `ESP_LOGI(TAG, "Free heap: %" PRIu32, esp_get_free_heap_size());`
- Check for memory leaks in custom code
- Reduce LVGL buffer sizes if needed
- Verify partition table has adequate space

## Testing MQTT Topics

Test your MQTT broker separately:

```bash
# Subscribe to all topics
mosquitto_sub -h 192.168.1.100 -t '#' -v

# Publish test data
mosquitto_pub -h 192.168.1.100 -t '/devices/outside-sensor/controls/temperature' -m '23.5'
mosquitto_pub -h 192.168.1.100 -t '/devices/humidity-sensor/controls/humidity' -m '57'
mosquitto_pub -h 192.168.1.100 -t '/devices/system_time/controls/current_time' -m '12:45:30'
mosquitto_pub -h 192.168.1.100 -t '/devices/system_time/controls/current_date' -m '2025-01-15'
```

## Useful Commands

No local ESP-IDF: build in Docker and flash with esptool (build dir lives in the `idf61-build` Docker volume, not in host `build/`):

```bash
# Build, flash, and print 30s of serial log
.claude/skills/flash-device/scripts/flash.sh --monitor 30
```

With a local ESP-IDF 6.x (`source ~/esp/v6.1/esp-idf/export.sh`):

```bash
# Build only
idf.py build

# Flash (rebuilds first if needed) — serial / USB ACM
idf.py -p PORT flash

# Flash via USB-JTAG when serial connect fails (needs host build/ from `idf.py build`)
openocd -f board/esp32c6-builtin.cfg \
  -c "program_esp build/bootloader/bootloader.bin 0x0 verify" \
  -c "program_esp build/partition_table/partition-table.bin 0x8000 verify" \
  -c "program_esp build/ESP32-C6-LCD-1.47-Test.bin 0x10000 verify reset exit"

# Monitor only
idf.py -p PORT monitor

# Build, flash, and monitor
idf.py -p PORT flash monitor

# Clean build
idf.py fullclean
idf.py build

# Edit config interactively (changes to sdkconfig are picked up by the next build)
idf.py menuconfig

# Apply changed sdkconfig.defaults (only used when sdkconfig doesn't exist; fullclean keeps sdkconfig)
rm sdkconfig && idf.py reconfigure

# Set target (already esp32c6; deletes sdkconfig and build/)
idf.py set-target esp32c6
```

Exit the monitor with `Ctrl+]`.

## Customization

### Change Display Colors

Edit `main/LVGL_UI/weather_station_ui.c`:
```c
lv_obj_set_style_text_color(label, lv_color_make(R, G, B), 0);
```

### Add New MQTT Topic

1. Add define to `config/app_config.h` (and `config/app_config.h.example` so others get it):
   ```c
   #define MQTT_TOPIC_NEW "/path/to/topic"
   ```

2. Subscribe in `main/Wireless/mqtt_handler.c`:
   ```c
   esp_mqtt_client_subscribe(mqtt_client, MQTT_TOPIC_NEW, 0);
   ```

3. Parse in `mqtt_event_handler()` (`topic` is a NUL-terminated copy):
   ```c
   } else if (strcmp(topic, MQTT_TOPIC_NEW) == 0) {
       // Handle new data
   }
   ```

4. Add field to `sensor_data_t` structure

5. Update UI in `main/LVGL_UI/weather_station_ui.c`

### Change Brightness Levels

Edit `main/LVGL_UI/weather_station_ui.c` → `weather_station_cycle_backlight()`:
- Modify `switch` cases
- Update modulo value: `backlight_level = (backlight_level + 1) % N;`

### Change Timezone or Night Mode

Edit `config/app_config.h`:
```c
#define TIMEZONE_OFFSET_HOURS  5   // For UTC+5
#define NIGHT_MODE_START_HOUR  23
#define NIGHT_MODE_END_HOUR    7
```

## Security Notes

- `config/app_config.h` is in `.gitignore` - **never commit it!**
- Always use `config/app_config.h.example` as template for new setups
- Consider using MQTT over TLS for production deployments
- Multiple boards are fine: each gets a unique MQTT client ID from its MAC

## Performance

- **LVGL Refresh**: 30ms period, redraws only changed labels (LCD SPI at 40MHz)
- **UI Update Rate**: 1 Hz (1 second)
- **Button Response**: ~50ms (debounced)
- **RGB Animation**: 50 FPS (20ms per frame)
- **MQTT**: Event-driven (no polling overhead)
- **Free Heap**: ~200KB typical (monitor with `esp_get_free_heap_size()`)

## License

SPDX-License-Identifier: CC0-1.0

---

**For AI assistants**: See `AGENTS.md` for detailed implementation context.
