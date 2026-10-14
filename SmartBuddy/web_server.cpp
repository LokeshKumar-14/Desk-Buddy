#include "web_server.h"
#include "web_page.h"
#include "config.h"
#include "settings_manager.h"
#include "storage_manager.h"
#include "task_manager.h"
#include "daily_manager.h"
#include "timer_manager.h"
#include "time_manager.h"
#include "wifi_manager.h"
#include "weather_manager.h"
#include "media_manager.h"
#include "wallpaper_manager.h"
#include "theme_manager.h"
#include "display_setup.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <lvgl.h>

static WebServer s_srv(80);
static bool      s_running = false;
static uint32_t  s_rebootAt = 0;

static void sendJson(int code, const JsonDocument& d) {
  String out;
  out.reserve(512);
  serializeJson(d, out);
  s_srv.sendHeader("Cache-Control", "no-store");
  s_srv.send(code, "application/json", out);
}
static void sendMsg(int code, const char* msg) {
  JsonDocument d;
  d["ok"] = (code >= 200 && code < 300);
  d["msg"] = msg;
  sendJson(code, d);
}

static bool kindFromArg(MediaKind* k) {
  String s = s_srv.arg("kind");
  if (s == "bg") { *k = MEDIA_BACKGROUNDS; return true; }
  if (s == "gal") { *k = MEDIA_IMAGES; return true; }
  return false;
}

// ---------------------------------------------------------------- state
static void handleState() {
  JsonDocument d;
  d["uptime"] = millis() / 1000;
  struct tm lt;
  JsonObject t = d["time"].to<JsonObject>();
  t["valid"] = time_valid();
  t["tz"] = settings_get().tz;
  if (time_local(&lt)) {
    char b[24]; strftime(b, sizeof(b), "%Y-%m-%d %H:%M:%S", &lt);
    t["str"] = b; t["epoch"] = (uint32_t)time_now();
  }
  JsonObject w = d["wifi"].to<JsonObject>();
  char ip[20]; wifi_ip_str(ip, sizeof(ip));
  w["connected"] = wifi_connected(); w["ssid"] = wifi_ssid(); w["ip"] = ip;
  w["rssi"] = wifi_rssi(); w["internet"] = wifi_internet_ok();
  JsonObject sd = d["sd"].to<JsonObject>();
  sd["ok"] = storage_ok();
  sd["totalMB"] = (uint32_t)(storage_total_bytes() / 1048576ULL);
  sd["freeMB"] = (uint32_t)(storage_free_bytes() / 1048576ULL);
  d["heap"] = ESP.getFreeHeap();
  d["psram"] = ESP.getFreePsram();
  const SbSettings& s = settings_get();
  JsonObject st = d["settings"].to<JsonObject>();
  st["theme"] = s.theme; st["tz"] = s.tz; st["slideshow"] = s.slideshowSec;
  st["timer"] = s.timerSec; st["city"] = s.city; st["place"] = s.place;
  st["brightness"] = s.brightness;
  char bc[8]; snprintf(bc, sizeof(bc), "%06lX", (unsigned long)(s.bgColor & 0xFFFFFF));
  st["bgColor"] = bc;
  st["bgColorOnly"] = s.bgColorOnly;
  WeatherData wd; WeatherStatus ws; bool stale;
  weather_get(&wd, &ws, &stale);
  JsonObject wx = d["weather"].to<JsonObject>();
  static const char* names[] = { "no_location", "waiting", "fetching", "ok", "error" };
  wx["status"] = names[ws]; wx["valid"] = wd.valid; wx["stale"] = stale; wx["err"] = weather_error();
  if (wd.valid) {
    wx["temp"] = wd.tempC; wx["hum"] = wd.humidity; wx["desc"] = weather_describe(wd.code);
    wx["place"] = wd.place; wx["updated"] = wd.updatedAt;
  }
  JsonObject tm = d["timer"].to<JsonObject>();
  static const char* tn[] = { "idle", "running", "paused", "done" };
  tm["state"] = tn[timer_state()]; tm["remaining"] = timer_remaining_ms(); tm["duration"] = timer_duration_sec();
  JsonArray ta = d["tasks"].to<JsonArray>();
  for (int i = 0; i < tasks_count(); i++) {
    const Task* k = tasks_at(i);
    JsonObject o = ta.add<JsonObject>();
    o["id"] = k->id; o["text"] = k->text; o["done"] = k->done; o["created"] = k->created; o["completed"] = k->completed;
  }
  d["maxTasks"] = SB_MAX_TASKS;
  JsonArray da = d["daily"].to<JsonArray>();
  daily_to_json(da);
  d["bg"] = wallpaper_current();
  d["bgColorMode"] = wallpaper_color_mode();
  d["galCount"] = media_count(MEDIA_IMAGES);
  d["bgCount"] = media_count(MEDIA_BACKGROUNDS);
  sendJson(200, d);
}

static void handleTz() {
  JsonDocument d;
  JsonArray a = d.to<JsonArray>();
  for (int i = 0; i < settings_tz_count(); i++) {
    JsonObject o = a.add<JsonObject>();
    o["label"] = settings_tz(i).label; o["tz"] = settings_tz(i).posix;
  }
  sendJson(200, d);
}

// ---------------------------------------------------------------- tasks
static void handleTaskAdd() {
  if (!storage_ok()) { sendMsg(503, "SD card not available - tasks cannot be saved"); return; }
  if (tasks_count() >= SB_MAX_TASKS) { sendMsg(409, "Task list is full"); return; }
  if (!tasks_add(s_srv.arg("text").c_str())) { sendMsg(400, "Empty task"); return; }
  sendMsg(200, "added");
}
static void handleTaskToggle() { sendMsg(tasks_toggle((uint32_t)s_srv.arg("id").toInt()) ? 200 : 404, "toggle"); }
static void handleTaskDelete() { sendMsg(tasks_delete((uint32_t)s_srv.arg("id").toInt()) ? 200 : 404, "delete"); }

// ---------------------------------------------------------------- daily tasks
static void handleDailyAdd() {
  if (!storage_ok()) { sendMsg(503, "SD card not available - daily tasks cannot be saved"); return; }
  if (!daily_add(s_srv.arg("text").c_str(), s_srv.arg("subs").c_str())) { sendMsg(400, "Daily task not added"); return; }
  sendMsg(200, "daily added");
}
static void handleDailySubAdd() {
  if (!daily_add_sub((uint32_t)s_srv.arg("id").toInt(), s_srv.arg("text").c_str())) { sendMsg(400, "Subtask not added"); return; }
  sendMsg(200, "subtask added");
}
static void handleDailyToggle() { sendMsg(daily_toggle_task((uint32_t)s_srv.arg("id").toInt()) ? 200 : 404, "daily toggle"); }
static void handleDailySubToggle() {
  sendMsg(daily_toggle_sub((uint32_t)s_srv.arg("id").toInt(), (uint32_t)s_srv.arg("sid").toInt()) ? 200 : 404, "sub toggle");
}
static void handleDailyDelete() { sendMsg(daily_delete_task((uint32_t)s_srv.arg("id").toInt()) ? 200 : 404, "daily delete"); }
static void handleDailySubDelete() {
  sendMsg(daily_delete_sub((uint32_t)s_srv.arg("id").toInt(), (uint32_t)s_srv.arg("sid").toInt()) ? 200 : 404, "sub delete");
}

// ---------------------------------------------------------------- weather / timer / settings
static void handleWxCity() {
  String c = s_srv.arg("city"); c.trim();
  if (c.length() > 47) { sendMsg(400, "Name too long"); return; }
  weather_set_city(c.c_str());
  sendMsg(200, c.length() ? "Location saved, updating..." : "Location cleared");
}
static void handleWxRefresh() { weather_request_refresh(); sendMsg(200, "refreshing"); }

static void handleTimer() {
  String cmd = s_srv.arg("cmd");
  if (cmd == "start" || cmd == "resume") timer_start();
  else if (cmd == "pause") timer_pause();
  else if (cmd == "reset") timer_reset();
  else if (cmd == "set") {
    long sec = s_srv.arg("sec").toInt();
    if (sec < 10 || sec > 21600) { sendMsg(400, "Duration must be 10 s .. 6 h"); return; }
    if (!timer_set_duration((uint32_t)sec)) { sendMsg(409, "Reset the timer first"); return; }
  } else { sendMsg(400, "Unknown command"); return; }
  sendMsg(200, "ok");
}

static void handleSettings() {
  if (s_srv.hasArg("theme")) {
    int t = s_srv.arg("theme").toInt() ? 1 : 0;
    if (t != settings_get().theme) {
      settings_edit().theme = (uint8_t)t;
      theme_apply((uint8_t)t);
      wallpaper_on_theme_changed();
    }
  }
  if (s_srv.hasArg("tz")) {
    String tz = s_srv.arg("tz"); tz.trim();
    if (!settings_tz_valid(tz.c_str())) { sendMsg(400, "Invalid time zone string"); return; }
    strlcpy(settings_edit().tz, tz.c_str(), sizeof(settings_get().tz));
    time_set_tz(settings_get().tz);
  }
  if (s_srv.hasArg("slideshow")) {
    long v = s_srv.arg("slideshow").toInt();
    settings_edit().slideshowSec = (uint16_t)constrain(v, 3, 300);
  }
  if (s_srv.hasArg("timer")) {
    long v = s_srv.arg("timer").toInt();
    v = constrain(v, 10, 21600);
    if (!timer_set_duration((uint32_t)v)) settings_edit().timerSec = (uint16_t)v;   // running: applies next reset
  }
  if (s_srv.hasArg("brightness")) {
    uint8_t v = (uint8_t)constrain(s_srv.arg("brightness").toInt(), 5, 100);
    settings_edit().brightness = v;
    display_set_brightness(v);
  }
  if (s_srv.hasArg("bgColor")) {
    String c = s_srv.arg("bgColor"); c.trim();
    if (c.startsWith("#")) c.remove(0, 1);
    if (c.length() != 6) { sendMsg(400, "Bad color"); return; }
    char* end = nullptr;
    uint32_t v = strtoul(c.c_str(), &end, 16);
    if (!end || *end != 0) { sendMsg(400, "Bad color"); return; }
    settings_edit().bgColor = v & 0xFFFFFF;
    if (settings_get().bgColorOnly) wallpaper_request_color();
  }
  if (s_srv.hasArg("bgColorOnly")) {
    bool on = s_srv.arg("bgColorOnly").toInt() != 0;
    settings_edit().bgColorOnly = on;
    if (on) { settings_edit().bg[0] = 0; wallpaper_request_color(); }
  }
  sendMsg(200, "saved");
}

static void handleReboot() {
  sendMsg(200, "rebooting");
  s_rebootAt = millis() + 600;
}

// ---------------------------------------------------------------- media
static void handleFiles() {
  MediaKind k;
  if (!kindFromArg(&k)) { sendMsg(400, "bad kind"); return; }
  media_refresh(k);
  JsonDocument d;
  JsonArray a = d.to<JsonArray>();
  for (int i = 0; i < media_count(k); i++) {
    JsonObject o = a.add<JsonObject>();
    o["name"] = media_name(k, i); o["size"] = media_size(k, i);
    if (k == MEDIA_BACKGROUNDS) o["active"] = (strcmp(media_name(k, i), wallpaper_current()) == 0);
  }
  sendJson(200, d);
}

static void handleFile() {
  MediaKind k; char path[128];
  String name = s_srv.arg("name");
  if (!kindFromArg(&k) || !media_path(k, name.c_str(), path, sizeof(path))) { sendMsg(404, "not found"); return; }
  File f = SD.open(path, FILE_READ);
  if (!f) { sendMsg(404, "not found"); return; }
  s_srv.sendHeader("Cache-Control", "max-age=300");
  s_srv.streamFile(f, "image/jpeg");
  f.close();
}

static void handleFileDelete() {
  MediaKind k;
  String name = s_srv.arg("name");
  if (!kindFromArg(&k) || media_find(k, name.c_str()) < 0) { sendMsg(404, "not found"); return; }
  char nm[SB_MEDIA_NAME_MAX]; strlcpy(nm, name.c_str(), sizeof(nm));
  if (!media_delete(k, nm)) { sendMsg(500, "delete failed"); return; }
  if (k == MEDIA_BACKGROUNDS) wallpaper_on_deleted(nm);
  sendMsg(200, "deleted");
}

static void handleBgSelect() {
  if (!wallpaper_request_select(s_srv.arg("name").c_str())) { sendMsg(404, "not found"); return; }
  sendMsg(200, "applying");
}
static void handleBgNext() { wallpaper_request_next(); sendMsg(200, "next"); }
static void handleBgColor() {
  settings_edit().bgColorOnly = true;
  settings_edit().bg[0] = 0;
  wallpaper_request_color();
  sendMsg(200, "color background");
}

// upload state
static File     s_upFile;
static bool     s_upActive = false;
static char     s_upErr[64];
static char     s_upRaw[96];
static char     s_upFinal[SB_MEDIA_NAME_MAX];
static size_t   s_upSize = 0;
static uint8_t  s_upChunks = 0;
static MediaKind s_upKind = MEDIA_IMAGES;

static void upFail(const char* m) { if (!s_upErr[0]) strlcpy(s_upErr, m, sizeof(s_upErr)); }

static void handleUploadData() {
  HTTPUpload& up = s_srv.upload();
  if (up.status == UPLOAD_FILE_START) {
    s_upActive = true; s_upErr[0] = 0; s_upFinal[0] = 0; s_upSize = 0; s_upChunks = 0;
    strlcpy(s_upRaw, up.filename.c_str(), sizeof(s_upRaw));
    if (s_upFile) s_upFile.close();
    if (!kindFromArg(&s_upKind)) { upFail("bad kind"); return; }
    if (!storage_ok()) { upFail("SD card not available"); return; }
    char clean[SB_MEDIA_NAME_MAX];
    if (!media_sanitize_name(s_upRaw, clean, sizeof(clean))) { upFail("Only .jpg / .jpeg files are accepted"); return; }
    if (storage_free_bytes() < SB_UPLOAD_MAX_BYTES + 1048576ULL) { upFail("Not enough free space on SD card"); return; }
    char tmp[96];
    media_upload_tmp_path(s_upKind, tmp, sizeof(tmp));
    if (SD.exists(tmp)) SD.remove(tmp);
    s_upFile = SD.open(tmp, FILE_WRITE);
    if (!s_upFile) upFail("Cannot create file on SD card");
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (s_upErr[0] || !s_upFile) return;
    if (s_upSize == 0 && (up.currentSize < 3 || up.buf[0] != 0xFF || up.buf[1] != 0xD8 || up.buf[2] != 0xFF)) {
      upFail("Not a JPEG image"); return;
    }
    s_upSize += up.currentSize;
    if (s_upSize > SB_UPLOAD_MAX_BYTES) { upFail("File too large (max 3 MB)"); return; }
    if (s_upFile.write(up.buf, up.currentSize) != up.currentSize) { upFail("SD write failed"); return; }
    if ((++s_upChunks & 0x0F) == 0) lv_timer_handler();      // keep the screen alive during big uploads
  } else if (up.status == UPLOAD_FILE_END) {
    if (s_upFile) { s_upFile.flush(); s_upFile.close(); }
    if (!s_upErr[0] && s_upSize < 1024) upFail("File too small / empty");
    if (!s_upErr[0]) {
      if (!media_upload_commit(s_upKind, s_upRaw, s_upFinal, sizeof(s_upFinal))) upFail("Could not store file");
    } else media_upload_discard(s_upKind);
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    if (s_upFile) s_upFile.close();
    media_upload_discard(s_upKind);
    upFail("Upload aborted");
  }
}

static void handleUploadDone() {
  if (!s_upActive) { sendMsg(400, "No file received"); return; }
  s_upActive = false;
  if (s_upErr[0]) { sendMsg(400, s_upErr); return; }
  if (s_upKind == MEDIA_BACKGROUNDS && s_srv.arg("select") == "1") wallpaper_request_select(s_upFinal);
  JsonDocument d;
  d["ok"] = true; d["name"] = s_upFinal; d["kind"] = (s_upKind == MEDIA_BACKGROUNDS) ? "bg" : "gal";
  sendJson(200, d);
}

// ---------------------------------------------------------------- lifecycle
void web_begin() {
  if (s_running) return;
  s_srv.on("/", HTTP_GET, []() { s_srv.send_P(200, "text/html", SB_WEB_PAGE); });
  s_srv.on("/api/state", HTTP_GET, handleState);
  s_srv.on("/api/tz", HTTP_GET, handleTz);
  s_srv.on("/api/task/add", HTTP_POST, handleTaskAdd);
  s_srv.on("/api/task/toggle", HTTP_POST, handleTaskToggle);
  s_srv.on("/api/task/delete", HTTP_POST, handleTaskDelete);
  s_srv.on("/api/daily/add", HTTP_POST, handleDailyAdd);
  s_srv.on("/api/daily/sub/add", HTTP_POST, handleDailySubAdd);
  s_srv.on("/api/daily/toggle", HTTP_POST, handleDailyToggle);
  s_srv.on("/api/daily/sub/toggle", HTTP_POST, handleDailySubToggle);
  s_srv.on("/api/daily/delete", HTTP_POST, handleDailyDelete);
  s_srv.on("/api/daily/sub/delete", HTTP_POST, handleDailySubDelete);
  s_srv.on("/api/weather/city", HTTP_POST, handleWxCity);
  s_srv.on("/api/weather/refresh", HTTP_POST, handleWxRefresh);
  s_srv.on("/api/timer", HTTP_POST, handleTimer);
  s_srv.on("/api/settings", HTTP_POST, handleSettings);
  s_srv.on("/api/reboot", HTTP_POST, handleReboot);
  s_srv.on("/api/files", HTTP_GET, handleFiles);
  s_srv.on("/api/file", HTTP_GET, handleFile);
  s_srv.on("/api/file/delete", HTTP_POST, handleFileDelete);
  s_srv.on("/api/bg/select", HTTP_POST, handleBgSelect);
  s_srv.on("/api/bg/next", HTTP_POST, handleBgNext);
  s_srv.on("/api/bg/color", HTTP_POST, handleBgColor);
  s_srv.on("/api/upload", HTTP_POST, handleUploadDone, handleUploadData);
  s_srv.onNotFound([]() { sendMsg(404, "not found"); });
  s_srv.begin();
  if (MDNS.begin(SB_HOSTNAME)) MDNS.addService("http", "tcp", 80);
  s_running = true;
  Serial.println("[WEB] server started");
}

void web_loop() {
  if (!s_running) return;
  s_srv.handleClient();
  if (s_rebootAt && (int32_t)(millis() - s_rebootAt) >= 0) {
    settings_flush();
    delay(100);
    ESP.restart();
  }
}

bool web_running() { return s_running; }
