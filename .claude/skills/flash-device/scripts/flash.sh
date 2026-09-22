#!/usr/bin/env bash
# Build (ESP-IDF 6.1 in Docker) and flash ESP32-C6 weather station firmware.
# Docker on macOS can't reach USB serial, so the build runs in a container and
# esptool (via uvx) flashes from the host.
set -euo pipefail

IDF_IMAGE="${IDF_IMAGE:-espressif/idf:v6.1}"
BUILD_VOLUME="${BUILD_VOLUME:-idf61-build}"   # named volume: incremental rebuilds, no host build/
CHIP="esp32c6"
BAUD=460800

PORT=""
MONITOR_SECS=0
PROJECT_ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"

usage() {
  cat <<'EOF'
Usage: flash.sh [--port /dev/cu.usbmodemXXXX] [--monitor [SECONDS]]

Builds this project with ESP-IDF 6.1 in Docker and flashes it with esptool.
--monitor captures the serial log for SECONDS (default 20) after a reset and prints it.
Set IDF_PATH to a local ESP-IDF 6.x install to build/flash with idf.py instead of Docker.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --port)
      PORT="${2:-}"
      shift 2
      ;;
    --monitor)
      MONITOR_SECS=20
      if [[ "${2:-}" =~ ^[0-9]+$ ]]; then MONITOR_SECS="$2"; shift; fi
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

list_ports() {
  # Prefer usbmodem (ESP USB-Serial/JTAG); fall back to common USB serial names.
  local p
  for p in /dev/cu.usbmodem* /dev/cu.wchusbserial* /dev/cu.SLAB_USBtoUART* /dev/cu.usbserial*; do
    [[ -e "$p" ]] && echo "$p"
  done
  return 0
}

if [[ -z "$PORT" ]]; then
  PORTS=()
  while IFS= read -r line; do
    [[ -n "$line" ]] && PORTS+=("$line")
  done < <(list_ports)
  if [[ ${#PORTS[@]} -eq 0 ]]; then
    echo "No ESP USB serial port found under /dev/cu.usbmodem*." >&2
    echo "Plug in the board (unplug the other device if flashing a second unit)." >&2
    exit 1
  fi
  if [[ ${#PORTS[@]} -gt 1 ]]; then
    echo "Multiple serial ports found:" >&2
    printf '  %s\n' "${PORTS[@]}" >&2
    echo "Re-run with: flash.sh --port /dev/cu.usbmodemXXXX" >&2
    exit 1
  fi
  PORT="${PORTS[0]}"
fi

if [[ ! -e "$PORT" ]]; then
  echo "Port not found: $PORT" >&2
  exit 1
fi

cd "$PROJECT_ROOT"
echo "Project: $PROJECT_ROOT"
echo "Port:    $PORT"

if [[ -n "${IDF_PATH:-}" && -f "$IDF_PATH/export.sh" ]]; then
  echo "Build:   local ESP-IDF at $IDF_PATH"
  # shellcheck disable=SC1091
  source "$IDF_PATH/export.sh" >/dev/null
  idf.py -p "$PORT" build flash
else
  command -v docker >/dev/null || { echo "docker not found (or set IDF_PATH to a local ESP-IDF 6.x)" >&2; exit 1; }
  command -v uvx >/dev/null || { echo "uvx not found (needed to run esptool)" >&2; exit 1; }
  echo "Build:   $IDF_IMAGE (docker volume $BUILD_VOLUME)"

  OUT="$(mktemp -d)"
  trap 'rm -rf "$OUT"' EXIT
  # Build, then copy flash_args + referenced .bin files out of the volume
  docker run --rm -v "$PROJECT_ROOT":/project -v "$BUILD_VOLUME":/tmp/build -v "$OUT":/out -w /project "$IDF_IMAGE" \
    bash -c 'idf.py -B /tmp/build build >/tmp/build.log 2>&1 || { tail -60 /tmp/build.log; exit 1; }
             grep -E "warning:|Project build complete|binary size" /tmp/build.log | grep -v "^--" || true
             cd /tmp/build && cp flash_args /out/ && tar cf - $(grep -o "[^ ]*\.bin" flash_args) | tar xf - -C /out' \
    2>&1 | grep -v -E '^(Checking|Python |"python3"|Activating|Setting IDF_PATH|\* |Done!|Go to|  idf.py build|$)'   # drop image's export.sh banner

  (cd "$OUT" && uvx esptool --chip "$CHIP" -p "$PORT" -b "$BAUD" \
      --before default-reset --after hard-reset write-flash $(tr '\n' ' ' < flash_args)) \
    | grep -v -E "^Writing at "
fi

echo "Flashed: $PORT"

if [[ "$MONITOR_SECS" -gt 0 ]]; then
  echo "--- serial log (${MONITOR_SECS}s after reset) ---"
  # Non-interactive capture: reset via RTS, then read; USB-Serial-JTAG re-enumerates on reset.
  uvx --with pyserial python - "$PORT" "$MONITOR_SECS" <<'PY'
import serial, sys, time
port, secs = sys.argv[1], float(sys.argv[2])
s = serial.Serial(port, 115200, timeout=0.2)
s.dtr = False; s.rts = True; time.sleep(0.1); s.rts = False
s.close()
end = time.time() + secs
while time.time() < end:
    try:
        s = serial.Serial(port, 115200, timeout=0.2)
        while time.time() < end:
            sys.stdout.write(s.read(4096).decode("utf-8", "replace"))
            sys.stdout.flush()
    except (serial.SerialException, OSError):
        time.sleep(0.2)
PY
fi
