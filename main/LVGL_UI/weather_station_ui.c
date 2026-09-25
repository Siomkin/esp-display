#include "weather_station_ui.h"
#include "mqtt_handler.h"
#include "app_config.h"
#include "esp_log.h"
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ST7789.h"

static const char *TAG = "WeatherUI";

// UI elements
static lv_obj_t *time_label;
static lv_obj_t *date_label;
static lv_obj_t *temp_outside_label;
static lv_obj_t *temp_inside_label;
static lv_obj_t *humidity_label;
static lv_obj_t *illuminance_label;

// Outside temperature trend: one sample per minute, last 2 hours
#define TREND_POINTS     120
#define TREND_PERIOD_MS  (60 * 1000)
static lv_obj_t *trend_chart;
static lv_chart_series_t *trend_series;
static lv_timer_t *trend_timer;
static bool trend_started = false;  // First sample is taken as soon as data arrives

// State control
// Short press cycles 5% -> 10% -> 25% -> 100% (auto night mode) -> 0% -> 1% -> 5%
static const uint8_t levels[] = {0, 1, 5, 10, 25, 100};
#define LEVEL_COUNT  (sizeof(levels) / sizeof(levels[0]))
#define AUTO_LEVEL   5  // Only this level follows night mode
#define BOOT_LEVEL   2  // 5%
static uint8_t backlight_level = BOOT_LEVEL;
static int8_t night_mode = -1; // -1 unknown, 0 day, 1 night; BK_Light only on change

// lv_label_set_text() always invalidates, so skip identical text to avoid redraw + SPI flush
static void set_text_if_changed(lv_obj_t *label, const char *text)
{
    if (strcmp(lv_label_get_text(label), text) != 0) {
        lv_label_set_text(label, text);
    }
}

static void ui_update_timer_cb(lv_timer_t *timer)
{
    weather_station_ui_update();
}

static void trend_sample_cb(lv_timer_t *timer)
{
    sensor_data_t data;
    mqtt_get_sensor_data(&data);
    // Values in tenths of a degree; gaps (no data yet) are not drawn
    lv_chart_set_next_value(trend_chart, trend_series,
                            data.temp_outside_valid ? (int32_t)lroundf(data.temp_outside * 10) : LV_CHART_POINT_NONE);

    // Fit the Y range to the data, at least 1 °C tall so sensor noise doesn't look like a trend
    int32_t *y = lv_chart_get_series_y_array(trend_chart, trend_series);
    int32_t lo = INT32_MAX, hi = INT32_MIN;
    for (int i = 0; i < TREND_POINTS; i++) {
        if (y[i] == LV_CHART_POINT_NONE) continue;
        if (y[i] < lo) lo = y[i];
        if (y[i] > hi) hi = y[i];
    }
    if (lo > hi) return;  // No samples yet
    int32_t pad = (hi - lo < 10) ? (10 - (hi - lo) + 1) / 2 : 1;
    lv_chart_set_axis_range(trend_chart, LV_CHART_AXIS_PRIMARY_Y, lo - pad, hi + pad);
}

void weather_station_ui_init(void)
{
    // Get the active screen
    lv_obj_t *scr = lv_screen_active();
    
    // Set black background
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    
    /*******************************************
     * Time Display (Top Left, Large)
     *******************************************/
    time_label = lv_label_create(scr);
    lv_label_set_text(time_label, "00:00");
    lv_obj_set_style_text_color(time_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_48, 0);
    lv_obj_align(time_label, LV_ALIGN_TOP_LEFT, 10, 10);
    
    /*******************************************
     * Date Display (Below Time)
     *******************************************/
    date_label = lv_label_create(scr);
    lv_label_set_text(date_label, "MON, 01 JAN");
    lv_obj_set_style_text_color(date_label, lv_color_make(0, 200, 255), 0); // Cyan
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_20, 0);
    lv_obj_align_to(date_label, time_label, LV_ALIGN_OUT_BOTTOM_LEFT, 5, 0);
    
    /*******************************************
     * Outside Temperature (Top Right, Large)
     *******************************************/
    temp_outside_label = lv_label_create(scr);
    lv_label_set_text(temp_outside_label, "--°C");
    lv_obj_set_style_text_color(temp_outside_label, lv_color_make(255, 215, 0), 0); // Gold
    lv_obj_set_style_text_font(temp_outside_label, &lv_font_montserrat_48, 0); // Large for visibility
    lv_obj_align(temp_outside_label, LV_ALIGN_TOP_RIGHT, -10, 10);

    /*******************************************
     * Inside Temperature (Below Outside, Orange)
     *******************************************/
    temp_inside_label = lv_label_create(scr);
    lv_label_set_text(temp_inside_label, "--°C");
    lv_obj_set_style_text_color(temp_inside_label, lv_color_make(255, 165, 0), 0); // Orange
    lv_obj_set_style_text_font(temp_inside_label, &lv_font_montserrat_28, 0); 
    lv_obj_align(temp_inside_label, LV_ALIGN_TOP_RIGHT, -10, 70);
    
    /*******************************************
     * Humidity Display (Bottom Left)
     *******************************************/
    humidity_label = lv_label_create(scr);
    lv_label_set_text(humidity_label, "Hum: --%");
    lv_obj_set_style_text_color(humidity_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(humidity_label, &lv_font_montserrat_20, 0);
    lv_obj_align(humidity_label, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    
    /*******************************************
     * Illuminance Display (Bottom Right)
     *******************************************/
    illuminance_label = lv_label_create(scr);
    lv_label_set_text(illuminance_label, "0 lx");
    lv_obj_set_style_text_color(illuminance_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(illuminance_label, &lv_font_montserrat_32, 0);
    lv_obj_align(illuminance_label, LV_ALIGN_BOTTOM_RIGHT, -10, -10);

    /*******************************************
     * Outside temperature trend (between date and humidity)
     *******************************************/
    trend_chart = lv_chart_create(scr);
    lv_obj_set_size(trend_chart, 175, 46);
    lv_obj_align(trend_chart, LV_ALIGN_TOP_LEFT, 10, 88);
    lv_chart_set_type(trend_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(trend_chart, TREND_POINTS);
    lv_chart_set_update_mode(trend_chart, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_div_line_count(trend_chart, 0, 0);
    lv_obj_set_style_bg_opa(trend_chart, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(trend_chart, 0, 0);
    lv_obj_set_style_pad_all(trend_chart, 2, 0);  // Room for half a dot at the edges
    lv_obj_set_style_line_width(trend_chart, 2, LV_PART_ITEMS);
    lv_obj_set_style_size(trend_chart, 4, 4, LV_PART_INDICATOR);  // Dot per sample: visible from the first minute
    trend_series = lv_chart_add_series(trend_chart, lv_color_make(255, 215, 0), LV_CHART_AXIS_PRIMARY_Y); // Gold, like outside temp
    lv_chart_set_all_values(trend_chart, trend_series, LV_CHART_POINT_NONE);

    lv_timer_create(ui_update_timer_cb, 1000, NULL);
    trend_timer = lv_timer_create(trend_sample_cb, TREND_PERIOD_MS, NULL);

    ESP_LOGI(TAG, "Weather station UI initialized");
}

void weather_station_cycle_backlight(void)
{
    backlight_level = (backlight_level + 1) % LEVEL_COUNT;
    if (backlight_level == AUTO_LEVEL) {
        night_mode = -1;  // Re-evaluate on next UI update
    }
    BK_Light(levels[backlight_level]);
    ESP_LOGI(TAG, "Backlight: %u%%%s", levels[backlight_level],
             backlight_level == AUTO_LEVEL ? " (Auto mode enabled)" : "");
}

void weather_station_backlight_on(void)
{
    BK_Light(levels[BOOT_LEVEL]);
}

void weather_station_ui_update(void)
{
    // Get sensor data from MQTT
    sensor_data_t sensor_data;
    mqtt_get_sensor_data(&sensor_data);

    // Update Time and Date with TIMEZONE_OFFSET_HOURS applied
    if (sensor_data.date_valid && sensor_data.time_valid) {
        int year, month, day, hour, min;
        // Parse "YYYY-MM-DD" and "HH:MM"
        if (sscanf(sensor_data.date_str, "%d-%d-%d", &year, &month, &day) == 3 &&
            sscanf(sensor_data.time_str, "%d:%d", &hour, &min) == 2) {
            
            struct tm tm_utc = {0};
            tm_utc.tm_year = year - 1900;
            tm_utc.tm_mon = month - 1;
            tm_utc.tm_mday = day;
            tm_utc.tm_hour = hour;
            tm_utc.tm_min = min;
            tm_utc.tm_sec = 0;
            tm_utc.tm_isdst = 0;
            
            // Convert to timestamp (assuming default TZ is UTC/GMT)
            time_t t = mktime(&tm_utc);
            
            // Add timezone offset
            t += TIMEZONE_OFFSET_HOURS * 3600;
            
            // Convert back to broken-down time
            struct tm *tm_local = localtime(&t);
            
            // Update Text
            char time_out[16];
            strftime(time_out, sizeof(time_out), "%H:%M", tm_local);
            set_text_if_changed(time_label, time_out);
            
            char date_out[32];
            strftime(date_out, sizeof(date_out), "%a, %d %b", tm_local);
            for (int i = 0; date_out[i]; i++) {
                if (date_out[i] >= 'a' && date_out[i] <= 'z') {
                    date_out[i] = date_out[i] - 32;
                }
            }
            set_text_if_changed(date_label, date_out);
            
            // Auto Brightness (Night Mode)
            // 22:00 to 08:00 -> 1%, then restore previous brightness
            if (backlight_level == AUTO_LEVEL) {
                int h = tm_local->tm_hour;
                // Window may cross midnight (22-8) or not (1-6); START == END disables night mode
                int8_t is_night = (NIGHT_MODE_START_HOUR <= NIGHT_MODE_END_HOUR)
                    ? (h >= NIGHT_MODE_START_HOUR && h < NIGHT_MODE_END_HOUR)
                    : (h >= NIGHT_MODE_START_HOUR || h < NIGHT_MODE_END_HOUR);
                if (is_night != night_mode) {
                    night_mode = is_night;
                    // Night: 1%, day: restore saved brightness
                    BK_Light(is_night ? 1 : levels[AUTO_LEVEL]);
                }
            }
        }
    } else {
        // Fallback to raw data
        if (sensor_data.time_valid) set_text_if_changed(time_label, sensor_data.time_str);
        if (sensor_data.date_valid) set_text_if_changed(date_label, sensor_data.date_str);
    }
    
    // Start the trend immediately instead of an empty chart for the first minute
    if (!trend_started && sensor_data.temp_outside_valid) {
        trend_started = true;
        trend_sample_cb(NULL);
        lv_timer_reset(trend_timer);  // Next sample one period from now
    }

    // Update outside temperature
    if (sensor_data.temp_outside_valid) {
        char temp_str[16];
        snprintf(temp_str, sizeof(temp_str), "%.1f°C", sensor_data.temp_outside);
        set_text_if_changed(temp_outside_label, temp_str);
    }

    // Update inside temperature
    if (sensor_data.temp_inside_valid) {
        char temp_str[16];
        snprintf(temp_str, sizeof(temp_str), "%.1f°C", sensor_data.temp_inside);
        set_text_if_changed(temp_inside_label, temp_str);
    }
    
    // Update humidity
    if (sensor_data.humidity_valid) {
        char hum_str[32];
        snprintf(hum_str, sizeof(hum_str), "Hum: %.0f%%", sensor_data.humidity);
        set_text_if_changed(humidity_label, hum_str);
    }
    
    // Update illuminance
    if (sensor_data.illuminance_valid) {
        char lux_str[32];
        snprintf(lux_str, sizeof(lux_str), "%.0f lx", sensor_data.illuminance);
        set_text_if_changed(illuminance_label, lux_str);
    }
}
