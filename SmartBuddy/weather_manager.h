// Weather (Open-Meteo) + internet reachability, run in a background task so the
// UI never blocks on the network. No API key is required.
#pragma once
#include <Arduino.h>

struct WeatherData {
  bool     valid;          // at least one successful update since boot
  float    tempC;
  int      humidity;       // %
  int      code;           // WMO weather code
  bool     isDay;
  uint32_t updatedAt;      // epoch seconds of last success (0 = unknown)
  char     place[48];
};

enum WeatherStatus : uint8_t { WX_NO_LOCATION = 0, WX_WAITING, WX_FETCHING, WX_OK, WX_ERROR };
enum WxIcon : uint8_t { WXI_SUN, WXI_MOON, WXI_PARTLY, WXI_CLOUD, WXI_FOG, WXI_RAIN, WXI_SNOW, WXI_STORM };

void          weather_begin();                       // starts the background task
void          weather_loop();                        // main thread: applies resolved location
void          weather_set_city(const char* city);    // saves + triggers geocode + refresh
void          weather_request_refresh();
void          weather_get(WeatherData* d, WeatherStatus* st, bool* stale);
uint32_t      weather_version();
const char*   weather_describe(int wmoCode);
WxIcon        weather_icon(int wmoCode, bool isDay);
const char*   weather_error();                       // short text for the last failure
