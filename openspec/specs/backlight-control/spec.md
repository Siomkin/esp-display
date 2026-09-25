# backlight-control Specification

## Purpose
Defines the backlight brightness levels the user can select, how closely the panel matches each named level, and the automatic night-mode dimming.

## Requirements

### Requirement: Brightness level cycle
The device SHALL start at 5% brightness once the first frame is shown. Each short press SHALL move to the next level in the cycle 5% → 10% → 25% → 100% (auto) → 0% → 1% → 5%.

#### Scenario: Cycle from boot
- **WHEN** the user short-presses the BOOT button six times after boot
- **THEN** the backlight goes through 10%, 25%, 100%, 0%, 1% and back to 5%, in that order

### Requirement: Named brightness matches PWM duty
The backlight PWM duty cycle SHALL be the named percentage of full scale, within one PWM step. 0% SHALL switch the backlight fully off.

#### Scenario: Lowest non-zero level
- **WHEN** the backlight is set to 1%
- **THEN** the PWM duty is 1% of the 13-bit full scale (81 of 8191), not 2.1%

#### Scenario: Off
- **WHEN** the backlight is set to 0%
- **THEN** the PWM duty is 0

### Requirement: Night-mode window
In the 100% (auto) level, the device SHALL dim to 1% during the configured night window. It SHALL return to 100% outside the window. The window starts at `NIGHT_MODE_START_HOUR:00` and ends at `NIGHT_MODE_END_HOUR:00` local time, and SHALL be correct whether or not it crosses midnight. At every other level, night mode SHALL NOT change the brightness.

#### Scenario: Window crosses midnight
- **WHEN** start is 22, end is 8, the level is 100% (auto), and local time is 23:30
- **THEN** the backlight is at 1%

#### Scenario: Window within one day
- **WHEN** start is 1, end is 6, the level is 100% (auto), and local time is 12:00
- **THEN** the backlight is at 100%

#### Scenario: Window within one day, inside
- **WHEN** start is 1, end is 6, the level is 100% (auto), and local time is 03:00
- **THEN** the backlight is at 1%

#### Scenario: Fixed level ignores night mode
- **WHEN** the level is 25% and local time is inside the night window
- **THEN** the backlight stays at 25%
