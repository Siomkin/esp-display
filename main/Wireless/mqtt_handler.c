#include "mqtt_handler.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "MQTT";
static esp_mqtt_client_handle_t mqtt_client = NULL;
static sensor_data_t sensor_data = {0};
// Guards sensor_data: written by the MQTT task, read by the UI (LVGL) task
static StaticSemaphore_t sensor_lock_buf;
static SemaphoreHandle_t sensor_lock = NULL;

// Numeric sensor topics: one row per topic handles subscribe, parse and log.
// Date and time are strings and stay as explicit cases below.
static const struct {
    const char *topic;
    float *value;
    bool *valid;
    const char *label;
    int decimals;
    const char *unit;
} float_topics[] = {
    { MQTT_TOPIC_TEMP_OUTSIDE, &sensor_data.temp_outside, &sensor_data.temp_outside_valid, "Outside temperature", 1, "°C" },
    { MQTT_TOPIC_TEMP_INSIDE,  &sensor_data.temp_inside,  &sensor_data.temp_inside_valid,  "Inside temperature",  1, "°C" },
    { MQTT_TOPIC_HUMIDITY,     &sensor_data.humidity,     &sensor_data.humidity_valid,     "Humidity",            0, "%" },
    { MQTT_TOPIC_ILLUMINANCE,  &sensor_data.illuminance,  &sensor_data.illuminance_valid,  "Illuminance",         0, " lx" },
};
#define FLOAT_TOPIC_COUNT (sizeof(float_topics) / sizeof(float_topics[0]))

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        
        // Subscribe to all topics
        for (size_t i = 0; i < FLOAT_TOPIC_COUNT; i++) {
            esp_mqtt_client_subscribe(mqtt_client, float_topics[i].topic, 0);
        }
        esp_mqtt_client_subscribe(mqtt_client, MQTT_TOPIC_SYSTEM_DATE, 0);
        esp_mqtt_client_subscribe(mqtt_client, MQTT_TOPIC_SYSTEM_TIME, 0);
        
        ESP_LOGI(TAG, "Subscribed to sensor and time topics");
        break;
        
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
        
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);
        
        // Parse the data
        char topic[128] = {0};
        char data[64] = {0};
        
        // Drop oversized or empty payloads (empty = retained topic cleared); atof("") would read as 0
        if (event->topic_len >= sizeof(topic) || event->data_len >= sizeof(data) || event->data_len == 0) {
            ESP_LOGW(TAG, "Dropped message (topic_len=%d, data_len=%d)", event->topic_len, event->data_len);
            break;
        }
        memcpy(topic, event->topic, event->topic_len);
        memcpy(data, event->data, event->data_len);
        
        // Update sensor data based on topic
        xSemaphoreTake(sensor_lock, portMAX_DELAY);
        for (size_t i = 0; i < FLOAT_TOPIC_COUNT; i++) {
            if (strcmp(topic, float_topics[i].topic) == 0) {
                *float_topics[i].value = atof(data);
                *float_topics[i].valid = true;
                ESP_LOGI(TAG, "%s: %.*f%s", float_topics[i].label, float_topics[i].decimals,
                         *float_topics[i].value, float_topics[i].unit);
                break;
            }
        }
        if (strcmp(topic, MQTT_TOPIC_SYSTEM_DATE) == 0) {
            snprintf(sensor_data.date_str, sizeof(sensor_data.date_str), "%.15s", data);
            sensor_data.date_valid = true;
            ESP_LOGI(TAG, "Date: %s", sensor_data.date_str);
        } else if (strcmp(topic, MQTT_TOPIC_SYSTEM_TIME) == 0) {
            snprintf(sensor_data.time_str, sizeof(sensor_data.time_str), "%.15s", data);
            sensor_data.time_valid = true;
            ESP_LOGI(TAG, "Time: %s", sensor_data.time_str);
        }
        xSemaphoreGive(sensor_lock);
        break;
        
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
        
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

esp_err_t mqtt_client_init(void)
{
    sensor_lock = xSemaphoreCreateMutexStatic(&sensor_lock_buf);

    // Unique per board: a shared ID makes the broker drop the older session on every connect
    static char client_id[48];
    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_STA));
    snprintf(client_id, sizeof(client_id), "%s-%02x%02x%02x", MQTT_CLIENT_ID, mac[3], mac[4], mac[5]);
    ESP_LOGI(TAG, "Client ID: %s", client_id);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .credentials.client_id = client_id,
        // Empty string in app_config.h = anonymous broker: omit the field rather than send ""
        .credentials.username = MQTT_USERNAME[0] ? MQTT_USERNAME : NULL,
        .credentials.authentication.password = MQTT_PASSWORD[0] ? MQTT_PASSWORD : NULL,
        /* Survive WiFi drops during router reboot / AP firmware update */
        .session.keepalive = 30,
        .network.timeout_ms = 10000,
        .network.reconnect_timeout_ms = 5000,
        .network.disable_auto_reconnect = false,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_FAIL;
    }
    
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
    
    ESP_LOGI(TAG, "MQTT client started");
    return ESP_OK;
}

void mqtt_get_sensor_data(sensor_data_t *data)
{
    if (data != NULL && sensor_lock == NULL) {
        // UI starts before MQTT init (WiFi can block); report "no data yet"
        memset(data, 0, sizeof(sensor_data_t));
    } else if (data != NULL) {
        xSemaphoreTake(sensor_lock, portMAX_DELAY);
        memcpy(data, &sensor_data, sizeof(sensor_data_t));
        xSemaphoreGive(sensor_lock);
    }
}
