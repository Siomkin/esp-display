# Spec Delta

## Purpose

Defines when the BOOT button responds and how its actions interact with the periodic UI update, so a press always has the effect the user chose.

## ADDED Requirements

### Requirement: Button available from first frame
The BOOT button SHALL respond to presses as soon as the first UI frame is shown. It MUST NOT depend on WiFi or MQTT connecting.

#### Scenario: Access point is down at boot
- **WHEN** the device boots while the configured WiFi access point is unreachable
- **THEN** a short press changes the backlight within one second of the backlight first turning on

### Requirement: Press actions
A press shorter than 1 s SHALL cycle the backlight level. A press of 1 s or longer SHALL toggle the RGB LED. Presses SHALL be debounced for 50 ms.

#### Scenario: Short press
- **WHEN** the user presses and releases the BOOT button within 1 s
- **THEN** the backlight moves to the next level and the RGB LED is unchanged

#### Scenario: Long press
- **WHEN** the user holds the BOOT button for 1 s or longer and releases it
- **THEN** the RGB LED toggles and the backlight is unchanged

### Requirement: Press is not overridden by background updates
The backlight level chosen by a press SHALL remain in effect until the next press or the next night-mode transition. A UI update that was in progress at the moment of the press MUST NOT override it.

#### Scenario: Press during a night-mode update
- **WHEN** the level is 100% (auto) at night and the user presses to switch to 0% while the once-per-second UI update is running
- **THEN** the backlight ends at 0% and stays off
