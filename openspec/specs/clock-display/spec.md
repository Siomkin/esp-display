# clock-display Specification

## Purpose
Defines how the clock and date are derived from the UTC time and date published over MQTT, how the configured timezone offset is applied, and what the clock shows between and after updates.

## Requirements

### Requirement: Local time from MQTT UTC time
The clock SHALL show `HH:MM` local time, where local time is the MQTT UTC time plus `TIMEZONE_OFFSET_HOURS`. The date SHALL show `DAY, DD MON` in upper case for that local time.

#### Scenario: Offset crosses midnight
- **WHEN** the offset is 3, the MQTT date is 2026-09-25 and the MQTT time is 22:30
- **THEN** the clock shows `01:30` and the date shows `SAT, 26 SEP`

### Requirement: Offset applied before the date arrives
While a time has arrived but no valid date has, the clock SHALL still show local time (the offset applied to the hour, wrapping at 24). The date SHALL keep its previous text.

#### Scenario: Time without date
- **WHEN** the offset is 3, the MQTT time is 22:30 and no date has been received
- **THEN** the clock shows `01:30`

### Requirement: Clock keeps counting between updates
The clock SHALL advance on the device's own timer from the last received time, so it keeps showing the correct minute (and the date rolls over) when time messages are late or missing.

#### Scenario: Time messages stop briefly
- **WHEN** the last time message said 18:33 UTC and no further time message arrives for 10 minutes
- **THEN** the clock shows local time for 18:43 UTC

#### Scenario: New time message corrects drift
- **WHEN** a time message arrives while the clock is counting on its own
- **THEN** the clock shows the time from that message

### Requirement: Clock shows unknown after a long outage
After 1 hour with no time message, the clock SHALL show `--:--` until the next time message arrives.

#### Scenario: Controller offline for over an hour
- **WHEN** no time message has arrived for 61 minutes
- **THEN** the clock shows `--:--`
