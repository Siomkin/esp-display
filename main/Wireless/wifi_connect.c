#include "wifi_connect.h"
#include "freertos/timers.h"
#include <string.h>

static const char *TAG = "WiFi";
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
static bool s_is_connected = false;
static TimerHandle_t s_reconnect_timer = NULL;

/* Backoff: 1s, 2s, 5s, then 10s forever while AP is down */
static uint32_t wifi_reconnect_delay_ms(int retry_num)
{
    if (retry_num <= 1) {
        return 1000;
    }
    if (retry_num == 2) {
        return 2000;
    }
    if (retry_num == 3) {
        return 5000;
    }
    return 10000;
}

static void wifi_reconnect_timer_cb(TimerHandle_t xTimer)
{
    (void)xTimer;
    if (!s_is_connected) {
        ESP_LOGI(TAG, "Reconnecting to AP (attempt %d)...", s_retry_num);
        esp_wifi_connect();
    }
}

static void wifi_schedule_reconnect(void)
{
    s_retry_num++;
    uint32_t delay_ms = wifi_reconnect_delay_ms(s_retry_num);

    if (s_reconnect_timer == NULL) {
        s_reconnect_timer = xTimerCreate("wifi_reconn",
                                         pdMS_TO_TICKS(delay_ms),
                                         pdFALSE,
                                         NULL,
                                         wifi_reconnect_timer_cb);
        if (s_reconnect_timer == NULL) {
            ESP_LOGE(TAG, "Failed to create reconnect timer, connecting immediately");
            esp_wifi_connect();
            return;
        }
    } else {
        xTimerStop(s_reconnect_timer, 0);
        xTimerChangePeriod(s_reconnect_timer, pdMS_TO_TICKS(delay_ms), 0);
    }

    ESP_LOGI(TAG, "WiFi disconnected, retry %d in %lu ms", s_retry_num, (unsigned long)delay_ms);
    xTimerStart(s_reconnect_timer, 0);
}

static void event_handler(void* arg, esp_event_base_t event_base,
                         int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "WiFi started, connecting...");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *disc = (wifi_event_sta_disconnected_t *)event_data;
        s_is_connected = false;
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        ESP_LOGW(TAG, "Disconnected from AP (reason=%d)", disc ? disc->reason : -1);
        /* Keep retrying forever — router reboot / firmware update can take minutes */
        wifi_schedule_reconnect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        s_is_connected = true;
        if (s_reconnect_timer != NULL) {
            xTimerStop(s_reconnect_timer, 0);
        }
        xEventGroupClearBits(s_wifi_event_group, WIFI_FAIL_BIT);
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_connect_init(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialization finished, waiting for connection...");

    /* Wait for first IP; reconnect keeps running in background if this times out */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT,
            pdFALSE,
            pdFALSE,
            pdMS_TO_TICKS(WIFI_CONNECT_TIMEOUT_MS));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP SSID:%s", WIFI_SSID);
        return ESP_OK;
    }

    ESP_LOGW(TAG, "WiFi not ready yet (SSID:%s); will keep retrying in background", WIFI_SSID);
    return ESP_ERR_TIMEOUT;
}

bool wifi_is_connected(void)
{
    return s_is_connected;
}

EventGroupHandle_t wifi_get_event_group(void)
{
    return s_wifi_event_group;
}
