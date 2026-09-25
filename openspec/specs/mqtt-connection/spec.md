# mqtt-connection Specification

## Purpose
Defines how the weather station identifies and authenticates itself to the MQTT broker it reads sensor data from.

## Requirements

### Requirement: Broker authentication from config
The device SHALL send the `MQTT_USERNAME` and `MQTT_PASSWORD` values from `config/app_config.h` when it connects to the broker. When either value is an empty string, the device MUST omit that field and not send an empty one.

#### Scenario: Broker requires a login
- **WHEN** `MQTT_USERNAME` and `MQTT_PASSWORD` are set to credentials the broker accepts
- **THEN** the device connects, subscribes to the configured topics, and shows incoming sensor values

#### Scenario: Anonymous broker
- **WHEN** `MQTT_USERNAME` and `MQTT_PASSWORD` are both empty strings
- **THEN** the device connects without sending a username or password, as it does today

#### Scenario: Wrong credentials
- **WHEN** the broker rejects the configured credentials
- **THEN** the device logs the connection error and keeps retrying, as it does for other connection failures

### Requirement: Unique client ID per board
The device SHALL connect with the client ID `MQTT_CLIENT_ID` followed by `-` and the last three bytes of its WiFi station MAC in lowercase hex.

#### Scenario: Two boards share a config
- **WHEN** two boards built from the same `app_config.h` connect to one broker
- **THEN** they use different client IDs and neither drops the other's session
