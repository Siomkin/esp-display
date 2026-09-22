---
name: flash-device
description: >-
  Build and flash this ESP32-C6 weather station firmware to the connected
  Waveshare ESP32-C6-LCD-1.47 board, optionally capturing the boot log. Use when
  the user asks to flash, upload firmware, program the board (including a second
  device), or check that a change boots on real hardware.
---

# Flash Device

Run from the project root:

```bash
.claude/skills/flash-device/scripts/flash.sh                 # build + flash
.claude/skills/flash-device/scripts/flash.sh --monitor 30    # ...then print 30s of serial log (default 20)
.claude/skills/flash-device/scripts/flash.sh --port /dev/cu.usbmodemXXXX
```

Use a Bash timeout of at least 600000 ms: a cold Docker build compiles ~2000 files.

## Workflow

1. `ls /dev/cu.usbmodem* 2>/dev/null`
   - none → ask the user to plug in the board, stop
   - several → list them and ask which one; don't guess
   - second device → ask the user to swap boards (usually only one USB-Serial/JTAG port shows)
2. Run the script. It is the user's explicit request that authorizes flashing; don't flash unasked.
3. Verify with `--monitor` when the change touches boot, display, WiFi or MQTT. In the log, look for:
   - crashes: `Guru Meditation`, `abort()`, `assert failed`, boot loops (repeated `rst:`)
   - errors: lines starting `E (`
   - healthy: `Weather station UI initialized`, `Got IP`, `MQTT_EVENT_CONNECTED`, sensor values
4. Report: port, flashed or error, and what the boot log showed (quote error lines).

## How the script works

- Project targets **ESP-IDF 6.1**, not installed on this Mac → builds in Docker `espressif/idf:v6.1`,
  build dir in named volume `idf61-build` (no host `build/` needed).
- Copies `flash_args` + `.bin` files out of the volume, flashes with `uvx esptool`
  (Docker on macOS can't reach USB serial).
- `--monitor` resets via RTS and reads the port for N seconds with pyserial (via `uvx`);
  it tolerates the USB re-enumeration on reset. Never run interactive `idf.py monitor`.
- If `IDF_PATH` points at a local ESP-IDF **6.x**, it runs `idf.py -p PORT build flash` instead.
  A local ESP-IDF 5.x install is too old for this project.
