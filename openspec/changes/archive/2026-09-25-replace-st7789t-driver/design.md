# Design

## Context

What the panel receives today, in order:

1. The vendored driver's `init()` sends:
   - SLPOUT, then a 100 ms delay
   - `36h 00`
   - `3Ah 55`
   - `B0h 00 E8`
   - `B2 B7 BB C0 C2 C3 C4 C6`
   - `D0h A4 A1` (2 bytes since `fix-audit-behavior`)
   - `E0`, `E1`
   - `21h` (inversion on)
   - `29h` (display on)
2. Then `LCD_Init()` calls:
   - `mirror(true, false)`, which sends MADCTL with the BGR and MX bits
   - `set_gap(0, 34)`
   - `disp_on_off(true)`

ESP-IDF 6.1's `panel_st7789_init()` (read from `/opt/esp/idf/components/esp_lcd/src/esp_lcd_panel_st7789.c` in the `espressif/idf:v6.1` image) sends:

- SLPOUT, then a 100 ms delay
- MADCTL (the BGR bit from `rgb_ele_order`)
- COLMOD `55` for 16 bpp
- RAMCTRL `00 F0`, or `00 F8` when `data_endian = LCD_RGB_DATA_ENDIAN_LITTLE`

The only mismatch is RAMCTRL: `F8` vs this board's `E8`. They differ in bit 4, the EPF bits that control how 565 colour is expanded to 666.

## Goals / Non-Goals

**Goals:**
- The panel ends in the same register state as today.
- No new abstraction: one `static const` command table in `ST7789.c`.

**Non-Goals:**
- Retuning gamma, VCOM or porch values.
- Changing the orientation or gap handling.

## Decisions

1. **Built-in driver with `rgb_ele_order = BGR`, `data_endian = LITTLE`, `bits_per_pixel = 16`, `reset_gpio_num = 21`.** This matches today's final MADCTL, COLMOD and endianness. It also removes the unused config fields (`flags.reset_active_high`, `vendor_config`) the copy carried.
2. **Send this board's register writes after `esp_lcd_panel_init()`** from a table of `{cmd, data[14], len}`, looped through `esp_lcd_panel_io_tx_param()`. The table starts with `B0h 00 E8`, which overrides the built-in `F8` and restores today's EPF bits. Rejected: `vendor_config`, because the built-in ST7789 driver ignores it (unlike the ILI9341 and GC9A01 drivers).
3. **`0x21` becomes `esp_lcd_panel_invert_color(panel, true)`,** and the raw `0x29` is dropped. `LCD_Init()` already calls `disp_on_off(true)`, so the display-on command stays in one place.
4. **The draw path is unchanged.** Both drivers use the same CASET/RASET/RAMWR with gap offsets and a byte count based on bits per pixel. `LVGL_Driver.c` keeps `swap_bytes = false`.

## Risks / Trade-offs

- [A register differs and the image changes subtly (colour cast or gamma)] → Photograph a fixed screen before and after with the same backlight level and angle, and check it by eye, including the gold, orange and cyan text and the trend line.
- [The built-in MADCTL at init has the BGR bit where the vendor sent `00`] → Transient only: `mirror()` rewrites MADCTL a few milliseconds later, before the backlight turns on, so it never shows.
- [Rollback] → A single commit: `git revert` restores the vendored driver.
