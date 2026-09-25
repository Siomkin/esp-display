# Spec Delta

## Purpose

Defines how the sensor readings received over MQTT (outside and inside temperature, humidity, illuminance, and the outside-temperature trend) are shown, and how the screen signals missing or stale readings.

## ADDED Requirements

### Requirement: Placeholder until first reading
Each sensor field SHALL show a placeholder until its first reading arrives: `--°C` for both temperatures, `Hum: --%` for humidity and `-- lx` for illuminance.

#### Scenario: Boot without a broker
- **WHEN** the device boots and no MQTT message has arrived yet
- **THEN** the screen shows `--°C`, `--°C`, `Hum: --%` and `-- lx`

### Requirement: Stale readings are not shown as current
A sensor reading with no update for 5 minutes SHALL be shown as its placeholder until a new reading arrives. Each sensor SHALL be judged on its own.

#### Scenario: One sensor stops
- **WHEN** outside temperature has had no update for 5 minutes and the other sensors keep publishing
- **THEN** outside temperature shows `--°C` and the other fields keep showing their current values

#### Scenario: Broker or WiFi down
- **WHEN** no MQTT message has arrived for 5 minutes
- **THEN** all four sensor fields show their placeholders

#### Scenario: Sensor comes back
- **WHEN** a stale sensor publishes a new reading
- **THEN** its field shows the new value within one second

### Requirement: Trend shows only real samples
The outside-temperature trend SHALL add one point per minute from the current outside reading. A minute in which the reading is missing or stale SHALL be a gap in the line, not a repeat of an older value.

#### Scenario: Outside sensor stops for 10 minutes
- **WHEN** the outside temperature goes stale and stays stale for 10 minutes
- **THEN** the trend has a gap for the stale minutes and resumes when readings return
