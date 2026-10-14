#include "settings_manager.h"
#include "storage_manager.h"

static SbSettings s_set;
static bool       s_dirty = false;
static uint32_t   s_dirtyAt = 0;
static uint32_t   s_ver = 1;

static const TzPreset kTz[] = {
  { "India (IST)",             "IST-5:30" },
  { "UTC",                     "UTC0" },
  { "UK (London)",             "GMT0BST,M3.5.0/1,M10.5.0" },
  { "Central Europe",          "CET-1CEST,M3.5.0,M10.5.0/3" },
  { "UAE (Dubai)",             "<+04>-4" },
  { "Pakistan",                "PKT-5" },
  { "Bangladesh",              "<+06>-6" },
  { "Singapore / Malaysia",    "<+08>-8" },
  { "Japan",                   "JST-9" },
  { "Australia (Sydney)",      "AEST-10AEDT,M10.1.0,M4.1.0/3" },
  { "US Eastern",              "EST5EDT,M3.2.0,M11.1.0" },
  { "US Central",              "CST6CDT,M3.2.0,M11.1.0" },
  { "US Mountain",             "MST7MDT,M3.2.0,M11.1.0" },
  { "US Pacific",              "PST8PDT,M3.2.0,M11.1.0" },
};

int settings_tz_count() { return sizeof(kTz) / sizeof(kTz[0]); }
const TzPreset& settings_tz(int i) { return kTz[(i < 0 || i >= settings_tz_count()) ? 0 : i]; }
int settings_tz_find(const char* posix) {
  for (int i = 0; i < settings_tz_count(); i++) if (!strcmp(kTz[i].posix, posix)) return i;
  return -1;
}
bool settings_tz_valid(const char* p) {
  if (!p) return false;
  size_t n = strlen(p);
  if (n < 3 || n >= sizeof(s_set.tz)) return false;
  for (size_t i = 0; i < n; i++) {
    char c = p[i];
    if (!(isalnum((unsigned char)c) || strchr("+-:,./<>", c))) return false;
  }
  return true;
}

static void defaults() {
  memset(&s_set, 0, sizeof(s_set));
  s_set.theme = 0;
  strlcpy(s_set.tz, kTz[0].posix, sizeof(s_set.tz));
  s_set.slideshowSec = 8;
  s_set.timerSec = 25 * 60;
  s_set.hasLoc = false;
  s_set.brightness = 90;
  s_set.bgColor = 0x10243A;
  s_set.bgColorOnly = false;
}

static void copyStr(char* dst, size_t n, JsonVariantConst v) {
  const char* s = v.is<const char*>() ? v.as<const char*>() : nullptr;
  if (s) strlcpy(dst, s, n);
}

void settings_begin() {
  defaults();
  JsonDocument doc;
  if (storage_load_json(SB_FILE_SETTINGS, doc)) {
    int th = doc["theme"] | 0;
    s_set.theme = (th == 1) ? 1 : 0;
    const char* tz = doc["tz"] | "";
    if (settings_tz_valid(tz)) strlcpy(s_set.tz, tz, sizeof(s_set.tz));
    int ss = doc["slideshow"] | 8;
    s_set.slideshowSec = constrain(ss, 3, 300);
    int ts = doc["timer"] | 1500;
    s_set.timerSec = constrain(ts, 10, 21600);
    copyStr(s_set.city, sizeof(s_set.city), doc["city"]);
    copyStr(s_set.place, sizeof(s_set.place), doc["place"]);
    if (doc["lat"].is<float>() && doc["lon"].is<float>()) {
      float la = doc["lat"], lo = doc["lon"];
      if (la >= -90 && la <= 90 && lo >= -180 && lo <= 180 && s_set.city[0]) {
        s_set.lat = la; s_set.lon = lo; s_set.hasLoc = true;
      }
    }
    copyStr(s_set.bg, sizeof(s_set.bg), doc["bg"]);
    int br = doc["brightness"] | 90;
    s_set.brightness = (uint8_t)constrain(br, 5, 100);
    const char* bc = doc["bgColor"] | "";
    if (bc[0] == '#') bc++;
    if (strlen(bc) == 6) {
      char* end = nullptr;
      uint32_t v = strtoul(bc, &end, 16);
      if (end && *end == 0) s_set.bgColor = v & 0xFFFFFF;
    } else if (doc["bgColor"].is<uint32_t>()) {
      s_set.bgColor = ((uint32_t)doc["bgColor"]) & 0xFFFFFF;
    }
    s_set.bgColorOnly = doc["bgColorOnly"] | false;
  } else {
    Serial.println("[SET] using defaults");
  }
  s_dirty = false;
}

const SbSettings& settings_get() { return s_set; }
SbSettings& settings_edit() { s_dirty = true; s_dirtyAt = millis(); s_ver++; return s_set; }
uint32_t settings_version() { return s_ver; }

static bool saveNow() {
  JsonDocument doc;
  doc["theme"] = s_set.theme;
  doc["tz"] = s_set.tz;
  doc["slideshow"] = s_set.slideshowSec;
  doc["timer"] = s_set.timerSec;
  doc["city"] = s_set.city;
  doc["place"] = s_set.place;
  if (s_set.hasLoc) { doc["lat"] = s_set.lat; doc["lon"] = s_set.lon; }
  doc["bg"] = s_set.bg;
  doc["brightness"] = s_set.brightness;
  char bc[8];
  snprintf(bc, sizeof(bc), "%06lX", (unsigned long)(s_set.bgColor & 0xFFFFFF));
  doc["bgColor"] = bc;
  doc["bgColorOnly"] = s_set.bgColorOnly;
  return storage_save_json(SB_FILE_SETTINGS, doc);
}

void settings_flush() { if (s_dirty && saveNow()) s_dirty = false; }

void settings_loop() {
  if (!s_dirty) return;
  uint32_t now = millis();
  if (now - s_dirtyAt < 1500) return;
  if (saveNow()) s_dirty = false; else s_dirtyAt = now + 8000;   // retry later
}
