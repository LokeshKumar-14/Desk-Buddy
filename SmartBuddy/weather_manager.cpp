#include "weather_manager.h"
#include "config.h"
#include "settings_manager.h"
#include "time_manager.h"
#include "wifi_manager.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#if !SB_TLS_INSECURE
// Paste the PEM root CA for api.open-meteo.com / geocoding-api.open-meteo.com here.
static const char SB_OPEN_METEO_CA[] = "";
#endif

static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static WeatherData  s_wx;
static WeatherStatus s_status = WX_WAITING;
static char         s_err[48] = "";
static uint32_t     s_ver = 1;

// location config used by the task (copied from settings by the main thread)
struct LocCfg { char city[48]; float lat, lon; bool has; char place[48]; };
static LocCfg s_cfg;

// resolved location waiting to be stored by the main thread
static volatile bool s_resolved = false;
static LocCfg        s_resolvedCfg;

static volatile bool s_refresh = false;

// ---------------------------------------------------------------- helpers
static void setStatus(WeatherStatus st, const char* err = "") {
  portENTER_CRITICAL(&s_mux);
  if (s_status != st || strcmp(s_err, err)) s_ver++;
  s_status = st;
  strlcpy(s_err, err, sizeof(s_err));
  portEXIT_CRITICAL(&s_mux);
}

static void urlEncode(const char* in, char* out, size_t n) {
  size_t o = 0;
  for (; *in && o + 4 < n; in++) {
    unsigned char c = (unsigned char)*in;
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') out[o++] = (char)c;
    else { snprintf(out + o, 4, "%%%02X", c); o += 3; }
  }
  out[o] = 0;
}

static bool httpGet(const char* url, String& body, int* codeOut) {
  HTTPClient http;
  WiFiClientSecure sec;
  WiFiClient plain;
  bool https = strncmp(url, "https", 5) == 0;
  http.setTimeout(8000);
  http.setConnectTimeout(6000);
  http.useHTTP10(true);
  bool began;
  if (https) {
#if SB_TLS_INSECURE
    sec.setInsecure();
#else
    sec.setCACert(SB_OPEN_METEO_CA);
#endif
    began = http.begin(sec, url);
  } else {
    began = http.begin(plain, url);
  }
  if (!began) return false;
  int code = http.GET();
  if (codeOut) *codeOut = code;
  bool ok = (code == 200);
  if (ok) body = http.getString();
  http.end();
  return ok;
}

static bool internetReachable() {
  HTTPClient http;
  WiFiClient c;
  http.setTimeout(4000);
  http.setConnectTimeout(3000);
  if (!http.begin(c, "http://connectivitycheck.gstatic.com/generate_204")) return false;
  int code = http.GET();
  http.end();
  return code == 204 || code == 200;
}

static bool geocode(const char* city, LocCfg* out) {
  char q[48], enc[160], url[256];
  strlcpy(q, city, sizeof(q));
  char* comma = strchr(q, ',');
  if (comma) *comma = 0;
  urlEncode(q, enc, sizeof(enc));
  snprintf(url, sizeof(url),
           "https://geocoding-api.open-meteo.com/v1/search?name=%s&count=1&language=en&format=json", enc);
  String body; int code = 0;
  if (!httpGet(url, body, &code)) { setStatus(WX_ERROR, code > 0 ? "Geocoder error" : "No connection"); return false; }
  JsonDocument doc;
  if (deserializeJson(doc, body)) { setStatus(WX_ERROR, "Bad geocoder reply"); return false; }
  JsonObject r = doc["results"][0];
  if (r.isNull() || !r["latitude"].is<float>()) { setStatus(WX_ERROR, "Location not found"); return false; }
  out->lat = r["latitude"]; out->lon = r["longitude"];
  const char* name = r["name"] | city;
  const char* country = r["country"] | "";
  if (country[0]) snprintf(out->place, sizeof(out->place), "%s, %s", name, country);
  else strlcpy(out->place, name, sizeof(out->place));
  strlcpy(out->city, city, sizeof(out->city));
  out->has = true;
  return true;
}

static bool fetchForecast(const LocCfg& c) {
  char url[300];
  snprintf(url, sizeof(url),
           "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
           "&current=temperature_2m,relative_humidity_2m,weather_code,is_day&timezone=auto",
           c.lat, c.lon);
  String body; int code = 0;
  if (!httpGet(url, body, &code)) { setStatus(WX_ERROR, code > 0 ? "Weather server error" : "No connection"); return false; }
  JsonDocument doc;
  if (deserializeJson(doc, body)) { setStatus(WX_ERROR, "Bad weather reply"); return false; }
  JsonObject cur = doc["current"];
  if (cur.isNull() || !cur["temperature_2m"].is<float>()) { setStatus(WX_ERROR, "No weather data"); return false; }
  WeatherData d;
  memset(&d, 0, sizeof(d));
  d.valid = true;
  d.tempC = cur["temperature_2m"];
  d.humidity = cur["relative_humidity_2m"] | -1;
  d.code = cur["weather_code"] | 0;
  d.isDay = (cur["is_day"] | 1) != 0;
  d.updatedAt = (uint32_t)time_now();
  strlcpy(d.place, c.place[0] ? c.place : c.city, sizeof(d.place));
  portENTER_CRITICAL(&s_mux);
  s_wx = d;
  s_ver++;
  portEXIT_CRITICAL(&s_mux);
  setStatus(WX_OK);
  return true;
}

// ---------------------------------------------------------------- task
static void netTask(void*) {
  uint32_t lastNet = 0, lastWx = 0, lastTry = 0;
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(500));
    if (!wifi_connected()) { if (s_status != WX_FETCHING) setStatus(s_wx.valid ? s_status : WX_WAITING); continue; }
    uint32_t now = millis();
    if (lastNet == 0 || now - lastNet >= (wifi_internet_ok() ? 60000UL : 8000UL)) {
      lastNet = now ? now : 1;
      wifi_set_internet(internetReachable());
    }
    if (!wifi_internet_ok() || !time_valid()) continue;

    LocCfg cfg;
    portENTER_CRITICAL(&s_mux); cfg = s_cfg; portEXIT_CRITICAL(&s_mux);

    bool due = s_refresh || lastWx == 0 || (now - lastWx >= SB_WEATHER_INTERVAL_MS);
    if (!due && s_status == WX_ERROR && now - lastTry >= 60000UL) due = true;
    if (!due) continue;
    s_refresh = false;
    lastTry = now ? now : 1;

    if (!cfg.city[0] && !cfg.has) { setStatus(WX_NO_LOCATION); lastWx = lastTry; continue; }
    setStatus(WX_FETCHING);
    if (!cfg.has) {
      LocCfg r;
      memset(&r, 0, sizeof(r));
      if (!geocode(cfg.city, &r)) { lastWx = lastTry; continue; }   // retried by the 60 s error path
      portENTER_CRITICAL(&s_mux);
      s_cfg.lat = r.lat; s_cfg.lon = r.lon; s_cfg.has = true; strlcpy(s_cfg.place, r.place, sizeof(s_cfg.place));
      cfg = s_cfg;
      portEXIT_CRITICAL(&s_mux);
      s_resolvedCfg = r;
      s_resolved = true;
    }
    fetchForecast(cfg);
    lastWx = lastTry;                 // success: next in 20 min; error: retried in 60 s
  }
}

// ---------------------------------------------------------------- public API
void weather_begin() {
  const SbSettings& s = settings_get();
  memset(&s_cfg, 0, sizeof(s_cfg));
  strlcpy(s_cfg.city, s.city, sizeof(s_cfg.city));
  strlcpy(s_cfg.place, s.place, sizeof(s_cfg.place));
  s_cfg.lat = s.lat; s_cfg.lon = s.lon; s_cfg.has = s.hasLoc;
  memset(&s_wx, 0, sizeof(s_wx));
  if (!s.city[0]) s_status = WX_NO_LOCATION;
  xTaskCreatePinnedToCore(netTask, "sb_net", 14336, nullptr, 1, nullptr, 0);
}

void weather_loop() {
  if (!s_resolved) return;
  s_resolved = false;
  LocCfg r = s_resolvedCfg;
  if (strcmp(settings_get().city, r.city) != 0) return;       // user changed the city meanwhile
  SbSettings& e = settings_edit();
  e.lat = r.lat; e.lon = r.lon; e.hasLoc = true;
  strlcpy(e.place, r.place, sizeof(e.place));
}

void weather_set_city(const char* city) {
  SbSettings& e = settings_edit();
  strlcpy(e.city, city ? city : "", sizeof(e.city));
  e.hasLoc = false; e.place[0] = 0;
  portENTER_CRITICAL(&s_mux);
  memset(&s_cfg, 0, sizeof(s_cfg));
  strlcpy(s_cfg.city, e.city, sizeof(s_cfg.city));
  s_wx.valid = false;                       // old city's data must not stay on screen
  s_ver++;
  portEXIT_CRITICAL(&s_mux);
  s_refresh = true;
  setStatus(e.city[0] ? WX_WAITING : WX_NO_LOCATION);
}

void weather_request_refresh() { s_refresh = true; }
uint32_t weather_version() { return s_ver; }

void weather_get(WeatherData* d, WeatherStatus* st, bool* stale) {
  portENTER_CRITICAL(&s_mux);
  *d = s_wx; *st = s_status;
  portEXIT_CRITICAL(&s_mux);
  bool sl = false;
  if (d->valid && d->updatedAt && time_valid()) {
    uint32_t now = (uint32_t)time_now();
    sl = now > d->updatedAt && (now - d->updatedAt) > SB_WEATHER_STALE_SEC;
  }
  if (d->valid && *st == WX_ERROR) sl = true;
  if (stale) *stale = sl;
}

const char* weather_error() { return s_err; }

const char* weather_describe(int c) {
  switch (c) {
    case 0: return "Clear sky";
    case 1: return "Mainly clear";
    case 2: return "Partly cloudy";
    case 3: return "Overcast";
    case 45: case 48: return "Fog";
    case 51: case 53: case 55: return "Drizzle";
    case 56: case 57: return "Freezing drizzle";
    case 61: return "Light rain";
    case 63: return "Rain";
    case 65: return "Heavy rain";
    case 66: case 67: return "Freezing rain";
    case 71: case 73: case 75: case 77: return "Snow";
    case 80: case 81: case 82: return "Rain showers";
    case 85: case 86: return "Snow showers";
    case 95: return "Thunderstorm";
    case 96: case 99: return "Thunderstorm, hail";
    default: return "Unknown";
  }
}

WxIcon weather_icon(int c, bool day) {
  if (c == 0 || c == 1) return day ? WXI_SUN : WXI_MOON;
  if (c == 2) return WXI_PARTLY;
  if (c == 3) return WXI_CLOUD;
  if (c == 45 || c == 48) return WXI_FOG;
  if (c >= 95) return WXI_STORM;
  if ((c >= 71 && c <= 77) || c == 85 || c == 86) return WXI_SNOW;
  if (c >= 51) return WXI_RAIN;
  return WXI_CLOUD;
}
