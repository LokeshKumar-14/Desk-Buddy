/*---------------------------------------------------------------
 * SMART BUDDY
 * Elecrow CrowPanel 7.0" ESP32-S3
 *
 * Boot:
 * DISPLAY -> STORAGE -> UI -> WIFI / WEATHER
 *
 * Offline features remain available without Wi-Fi.
 *--------------------------------------------------------------*/

#include <Arduino.h>
#include <lvgl.h>

#include "config.h"
#include "display_setup.h"
#include "storage_manager.h"
#include "settings_manager.h"
#include "theme_manager.h"
#include "time_manager.h"
#include "task_manager.h"
#include "daily_manager.h"
#include "timer_manager.h"
#include "wifi_manager.h"
#include "weather_manager.h"
#include "wallpaper_manager.h"
#include "ui_dashboard.h"
#include "web_server.h"
#include "esp_heap_caps.h"

#define BUF_LINES 40
lv_color_t *buf1 = (lv_color_t*)heap_caps_malloc(800 * BUF_LINES * sizeof(lv_color_t),
                                                 MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);

static bool s_displayOk = false;

void setup() {
    delay(300);

    // 1. Initialize display, touch and LVGL.
    s_displayOk = display_begin();

    if (!s_displayOk) {
        return;
    }

    // 2. Initialize storage and settings.
    storage_begin();
    settings_begin();

    display_set_brightness(settings_get().brightness);

    // 3. Initialize theme and time.
    theme_init();
    theme_apply(settings_get().theme);

    time_begin(settings_get().tz);

    // 4. Initialize application managers.
    tasks_begin();
    daily_begin();
    timer_begin();
    wallpaper_begin();

    // 5. Create dashboard.
    ui_dashboard_create();

    // Process initial LVGL events.
    lv_timer_handler();

    // 6. Start network-dependent features.
#if !SB_DIAG_NO_WIFI
    wifi_begin();
    weather_begin();
#endif
}

void loop() {
    if (!s_displayOk) {
        delay(1000);
        return;
    }

    // Process LVGL, touch and animations.
    lv_timer_handler();

    // Wallpaper and image handling.
    wallpaper_loop();

    // Storage and settings.
    storage_loop();
    settings_loop();

    // Tasks, daily items and timer.
    tasks_loop();
    daily_loop();
    timer_loop();

    // Network and weather.
    wifi_loop();
    weather_loop();
    web_loop();

    // Handle Wi-Fi connection event.
    if (wifi_take_connected_event()) {
        time_start_sntp();
        web_begin();
    }

    // Process time synchronization event.
    time_take_valid_event();

    // Update dashboard values every 100 ms.
    static uint32_t lastUiUpdate = 0;
    uint32_t now = millis();

    if (now - lastUiUpdate >= 100) {
        lastUiUpdate = now;
        ui_dashboard_tick();
    }

    delay(5);
}