# Proposal

## Why

`main/LCD_Driver/Vernon_ST7789T/` (~300 lines) is a copied, older version of ESP-IDF's own ST7789 panel driver with this board's register writes hard-coded into its `init()` (AUDIT item 11). Its constructor works out MADCTL and COLMOD from the config and `init()` then ignores both, so setting `bits_per_pixel = 18` would silently break the display. ESP-IDF 6.1 ships `esp_lcd_new_panel_st7789()` with the same interface, so the copy is maintenance weight with no benefit.

## What Changes

- Use ESP-IDF's built-in `esp_lcd_new_panel_st7789()` in `LCD_Init()` (`main/LCD_Driver/ST7789.c`).
- Move this panel's register writes (RAMCTRL, porch, gate, VCOM, power and gamma) into a short table in `ST7789.c`, sent right after `esp_lcd_panel_init()`. Use `esp_lcd_panel_invert_color(panel, true)` in place of the raw `0x21`.
- Delete `main/LCD_Driver/Vernon_ST7789T/` and its entries in `main/CMakeLists.txt` and `ST7789.h`.
- No change in behaviour: the panel ends up with the same register values, so the picture, orientation, colours and backlight are unchanged.

## Capabilities

### New Capabilities
None.

### Modified Capabilities
None. This is a refactor with no spec-level behaviour change, so `skip_specs: true` is set.

## Impact

- `main/LCD_Driver/ST7789.c` and `.h`, `main/CMakeLists.txt`. Deletes `main/LCD_Driver/Vernon_ST7789T/`.
- Needs hardware verification, because a wrong register byte shows up only on the panel.
- No new dependencies: `esp_lcd` is already part of the build.
- Ticks AUDIT.md item 11.
