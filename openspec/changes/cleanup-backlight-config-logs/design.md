# Design

## Context

Four unrelated small edits, grouped because each is too small for its own change. None affects the screen, and each can be verified from the build output or the boot log.

## Goals / Non-Goals

**Goals:**
- One definition of the PWM resolution.
- An INFO-level boot log where WiFi, MQTT connect and errors are visible without scrolling.

**Non-Goals:**
- Changing log tags or the per-value parsed log line (`Outside temperature: 10.8°C`), which is useful at INFO.
- Adding a Kconfig option for log verbosity. Per-tag levels already exist through `esp_log_level_set` and menuconfig.

## Decisions

1. **Rename the LEDC macros rather than delete them.** `BK_LEDC_*` says what they're for. The old "HS" names are wrong because the channel is low speed. Rejected: inlining the `LEDC_*` constants, which would spread the timer and channel choice over two functions again.
2. **`ESP_LOGD` for the raw MQTT lines.** A developer who needs them can raise the `MQTT` tag to DEBUG in menuconfig without a code change. The error line uses `event->error_handle->error_type` and, for TCP transport errors, `esp_transport_sock_errno`, which gives a concrete reason when the broker is unreachable.
3. **Delete the dead config lines** instead of commenting them out. The build already reports them as unknown settings for this chip (`SPIRAM`), or the linker drops them (Montserrat 40).

## Risks / Trade-offs

- [Someone relied on the raw `TOPIC=`/`DATA=` lines when debugging topics] → Documented in the AGENTS.md Debugging Tips: set the `MQTT` tag to DEBUG.
- [The stale local `sdkconfig` keeps `MONTSERRAT_40=y`] → Harmless (the font isn't linked). The task regenerates it anyway.
