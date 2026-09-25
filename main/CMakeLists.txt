if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/../config/app_config.h")
    message(FATAL_ERROR "Missing config/app_config.h. Run: cp config/app_config.h.example config/app_config.h and edit it.")
endif()

idf_component_register(
                         SRCS "main.c" 
                              "LCD_Driver/ST7789.c"
                              "LVGL_Driver/LVGL_Driver.c"
                              "LVGL_UI/weather_station_ui.c"
                              "SD_Card/SD_SPI.c"
                              "RGB/RGB.c"
                              "Wireless/wifi_connect.c"
                              "Wireless/mqtt_handler.c"
                              "button_handler.c"

                         INCLUDE_DIRS 
                              "."
                              "../config"
                              "./LCD_Driver" 
                              "./LVGL_Driver" 
                              "./LVGL_UI" 
                              "./SD_Card"
                              "./RGB" 
                              "./Wireless"
                       )
