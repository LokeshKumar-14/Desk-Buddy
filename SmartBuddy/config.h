// Smart Buddy - central configuration (pins, paths, limits)
#pragma once
#include <Arduino.h>

// ---------------------------------------------------------------- hardware
// Display/touch pins live in display_setup.cpp and touch.h exactly as in the
// original Elecrow LVGL_Arduino7.0 example. Do not change them here.
#define SB_SCREEN_W        800
#define SB_SCREEN_H        480
#define SB_TFT_BL          2     // backlight pin (as in original sketch)
#define SB_PIN_AUX_OUT     38    // driven LOW at boot, as in original sketch
#define SB_I2C_SDA         19    // shared by PCA9557 + GT911 touch
#define SB_I2C_SCL         20

// SD card (SPI) - taken from the Elecrow "SD card lesson" for this board.
#define SB_SD_MOSI         11
#define SB_SD_MISO         13
#define SB_SD_SCK          12
#define SB_SD_CS           10
#define SB_SD_FREQ_HZ      8000000UL    // calmer SPI keeps RGB scan stable on upstream LovyanGFX

// Uncomment to use the original LVGL FULL render mode instead of DIRECT mode.
#define SB_RENDER_FULL

// ---------------------------------------------------------------- SD layout
#define SB_DIR_ROOT        "/SmartBuddy"
#define SB_DIR_IMAGES      "/SmartBuddy/images"
#define SB_DIR_BG          "/SmartBuddy/backgrounds"
#define SB_DIR_DATA        "/SmartBuddy/data"
#define SB_FILE_TASKS      "/SmartBuddy/data/tasks.json"
#define SB_FILE_SETTINGS   "/SmartBuddy/data/settings.json"
#define SB_FILE_DAILY      "/SmartBuddy/data/daily.json"

// ---------------------------------------------------------------- limits
#define SB_MAX_TASKS         40
#define SB_TASK_TEXT_MAX     80
#define SB_MAX_DAILY_TASKS   12
#define SB_MAX_DAILY_SUBS    8
#define SB_DAILY_TEXT_MAX    56
#define SB_DONE_EXPIRY_SEC   (12UL * 3600UL)   // completed tasks vanish after 12 h
#define SB_MAX_MEDIA_FILES   64
#define SB_MEDIA_NAME_MAX    48                // incl. NUL
#define SB_UPLOAD_MAX_BYTES  (3UL * 1024UL * 1024UL)
#define SB_JPEG_MAX_DIM      12000             // refuse absurd images

// ---------------------------------------------------------------- network
#define SB_HOSTNAME          "smartbuddy"      // http://smartbuddy.local/
#define SB_NTP_1             "pool.ntp.org"
#define SB_NTP_2             "time.google.com"
#define SB_NTP_3             "time.cloudflare.com"
#define SB_TIME_VALID_AFTER  1735689600UL      // 2025-01-01: anything older = not synced
#define SB_WEATHER_INTERVAL_MS (20UL * 60UL * 1000UL)
#define SB_WEATHER_STALE_SEC   (60UL * 60UL)
// 1 = HTTPS encrypted but server certificate NOT verified (simple, works on
// every ESP32 core). Set to 0 and fill SB_OPEN_METEO_CA in weather_manager.cpp
// to pin a CA certificate instead.
#define SB_TLS_INSECURE      1

// ---------------------------------------------------------------- UI
#define SB_WEEK_STARTS_MONDAY 1
#define SB_CLOCK_24H          1
#define SB_UI_TRANSITIONS     1     // 1 = fade between screens (slow: full-screen redraw every frame)
#define SB_SHOW_SECONDS       1     // 1 = show :SS next to the clock (redraws every second)
#define SB_TFT_BL 2
// Uncomment to print every LVGL redraw area to Serial (debug: what is refreshing the screen?)
// #define SB_DEBUG_FLUSH

// Redraw debugger: prints WHICH function/widget triggers each redraw (see debug_redraw.h).
// Comment this line out when you are done debugging.
// #define SB_DEBUG_REDRAW

// ---------------------------------------------------------------- display glitch isolation
// LVGL is idle (debugger proves it), so a shimmering/glitching screen comes from the RGB panel
// scan itself (PSRAM bandwidth shared with Wi-Fi / flash / CPU). Use these to find the cause:
#define SB_PCLK_HZ        10000000UL   // stable RGB scan for upstream LovyanGFX + Wi-Fi/SD load
#define SB_DIAG_NO_WIFI   0            // 1 = never start Wi-Fi / weather / web server (test only)
#define SB_WIFI_SLEEP     0            // 0 = fewer bursty wakeups; uses more power but reduces shimmer
#define SB_DIAG_NO_SD     0            // 1 = do not touch the SD card at all (test only)
#define SB_DIAG_NO_TOUCH  0            // 1 = no touch polling (test only, screen will not react)
// Upstream-LovyanGFX path only: 1 = LVGL renders into ONE full-size PSRAM buffer.
// 0 = small partial stripes. On this RGB panel, 0 is steadier because it avoids
// an extra full-screen PSRAM buffer competing with panel scanout.
#define SB_UPSTREAM_DIRECT 0
