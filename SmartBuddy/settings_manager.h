// Persistent user settings (/SmartBuddy/data/settings.json on the SD card).
#pragma once
#include <Arduino.h>
#include "config.h"

struct SbSettings {
  uint8_t  theme;                       // 0 = Dark Glass, 1 = Light Glass
  char     tz[48];                      // POSIX TZ string
  uint16_t slideshowSec;                // 3..300
  uint16_t timerSec;                    // default timer duration, 10..21600
  char     city[48];                    // user-entered location
  char     place[48];                   // resolved name (from geocoder)
  float    lat, lon;
  bool     hasLoc;
  char     bg[SB_MEDIA_NAME_MAX];       // selected wallpaper file ("" = default)
  uint8_t  brightness;                  // 5..100 percent
  uint32_t bgColor;                     // RGB color used when no wallpaper image is shown
  bool     bgColorOnly;                 // true = show color background instead of a picture
};

struct TzPreset { const char* label; const char* posix; };

void               settings_begin();                 // load (defaults if missing/corrupt)
void               settings_loop();                  // debounced save
const SbSettings&  settings_get();
SbSettings&        settings_edit();                  // mark dirty + bump version
uint32_t           settings_version();
void               settings_flush();                 // save now (before reboot)

int                settings_tz_count();
const TzPreset&    settings_tz(int i);
int                settings_tz_find(const char* posix);   // -1 if custom
bool               settings_tz_valid(const char* posix);
