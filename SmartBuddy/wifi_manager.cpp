#include "wifi_manager.h"
#include "secrets.h"
#include "config.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <Preferences.h>

static char s_ssid[33] = "";
static char s_pass[65] = "";
static WifiMgrState s_st = SBW_NO_CREDS;
static uint32_t s_attemptAt = 0, s_retryAt = 0, s_lostAt = 0;
static uint8_t  s_fails = 0;
static bool     s_connEvt = false;
static uint32_t s_ver = 1;
static volatile bool s_internet = false;
static bool     s_enabled = true;

#define MAX_SCAN 12
static char s_scan[MAX_SCAN][33];
static int  s_scanN = 0;
static int  s_scanState = -2;

static void startAttempt() {
  if (!s_ssid[0]) { s_st = SBW_NO_CREDS; s_ver++; return; }
  WiFi.disconnect(false);
  WiFi.begin(s_ssid, s_pass[0] ? s_pass : nullptr);
  s_attemptAt = millis();
  s_st = SBW_CONNECTING;
  s_ver++;
  Serial.printf("[WIFI] connecting to \"%s\"...\n", s_ssid);
}

void wifi_begin() {
  WiFi.persistent(false);             // we keep credentials ourselves
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(SB_WIFI_SLEEP ? true : false);   // see config.h
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  esp_wifi_set_ps(SB_WIFI_SLEEP ? WIFI_PS_MIN_MODEM : WIFI_PS_NONE);
  Preferences p;
  if (p.begin("sbwifi", true)) {
    p.getString("ssid", s_ssid, sizeof(s_ssid));
    p.getString("pass", s_pass, sizeof(s_pass));
    p.end();
  }
  if (!s_ssid[0]) {                   // first boot: documented defaults from secrets.h
    strlcpy(s_ssid, SB_WIFI_DEFAULT_SSID, sizeof(s_ssid));
    strlcpy(s_pass, SB_WIFI_DEFAULT_PASS, sizeof(s_pass));
  }
  startAttempt();
}

WifiMgrState wifi_state() { return s_st; }
bool wifi_connected() { return s_st == SBW_CONNECTED; }
const char* wifi_ssid() { return s_ssid; }
uint32_t wifi_version() { return s_ver; }
int wifi_rssi() { return wifi_connected() ? WiFi.RSSI() : 0; }
void wifi_set_internet(bool ok) { if (s_internet != ok) { s_internet = ok; s_ver++; } }
bool wifi_internet_ok() { return wifi_connected() && s_internet; }

void wifi_ip_str(char* buf, size_t n) {
  if (!wifi_connected()) { if (n) buf[0] = 0; return; }
  IPAddress ip = WiFi.localIP();
  snprintf(buf, n, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

uint32_t wifi_retry_in_sec() {
  if (s_st != SBW_FAILED) return 0;
  int32_t d = (int32_t)(s_retryAt - millis());
  return d > 0 ? (uint32_t)(d / 1000) + 1 : 0;
}

bool wifi_set_credentials(const char* ssid, const char* pass) {
  if (!ssid || !ssid[0] || strlen(ssid) > 32 || (pass && strlen(pass) > 64)) return false;
  strlcpy(s_ssid, ssid, sizeof(s_ssid));
  strlcpy(s_pass, pass ? pass : "", sizeof(s_pass));
  Preferences p;
  if (p.begin("sbwifi", false)) {
    p.putString("ssid", s_ssid);
    p.putString("pass", s_pass);
    p.end();
  }
  s_fails = 0; s_internet = false;
  startAttempt();
  return true;
}

bool wifi_enabled() { return s_enabled; }

void wifi_set_enabled(bool on) {
  if (on == s_enabled) return;
  s_enabled = on;
  s_internet = false; s_ver++;
  if (!on) {
    WiFi.disconnect(true, false);
    WiFi.mode(WIFI_OFF);
    s_st = SBW_NO_CREDS;
    Serial.println("[WIFI] radio OFF");
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(SB_WIFI_SLEEP ? true : false);
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    esp_wifi_set_ps(SB_WIFI_SLEEP ? WIFI_PS_MIN_MODEM : WIFI_PS_NONE);
    s_fails = 0;
    startAttempt();
    Serial.println("[WIFI] radio ON");
  }
}

void wifi_reconnect() { s_fails = 0; s_internet = false; startAttempt(); }
bool wifi_take_connected_event() { bool e = s_connEvt; s_connEvt = false; return e; }

void wifi_loop() {
  if (!s_enabled) return;
  static uint32_t lastPoll = 0;
  uint32_t now = millis();
  if (now - lastPoll >= 250) {
    lastPoll = now;
    wl_status_t ws = WiFi.status();
    switch (s_st) {
      case SBW_CONNECTING:
        if (ws == WL_CONNECTED) {
          s_st = SBW_CONNECTED; s_fails = 0; s_connEvt = true; s_ver++;
          Serial.println("[WIFI] connected");
        } else if (now - s_attemptAt > 20000UL) {
          static const uint8_t backoff[] = { 5, 10, 20, 30, 60 };
          uint8_t b = backoff[s_fails < 4 ? s_fails : 4];
          if (s_fails < 250) s_fails++;
          s_retryAt = now + b * 1000UL;
          s_st = SBW_FAILED; s_ver++;
          WiFi.disconnect(false);
          Serial.println("[WIFI] connection failed, will retry");
        }
        break;
      case SBW_CONNECTED:
        if (ws != WL_CONNECTED) {
          if (!s_lostAt) { s_lostAt = now; s_internet = false; s_ver++; }
          if (now - s_lostAt > 12000UL) { s_lostAt = 0; startAttempt(); }
        } else s_lostAt = 0;
        break;
      case SBW_FAILED:
        if ((int32_t)(now - s_retryAt) >= 0) startAttempt();
        break;
      default: break;
    }
  }
  if (s_scanState == -1) {
    int r = WiFi.scanComplete();
    if (r >= 0) {
      s_scanN = 0;
      for (int i = 0; i < r && s_scanN < MAX_SCAN; i++) {
        String s = WiFi.SSID(i);
        if (s.length() == 0 || s.length() > 32) continue;
        bool dup = false;
        for (int k = 0; k < s_scanN; k++) if (s.equals(s_scan[k])) dup = true;
        if (!dup) strlcpy(s_scan[s_scanN++], s.c_str(), 33);
      }
      WiFi.scanDelete();
      s_scanState = s_scanN;
      s_ver++;
    } else if (r == WIFI_SCAN_FAILED) { s_scanState = 0; s_scanN = 0; }
  }
}

void wifi_scan_start() { if (s_scanState == -1) return; s_scanState = -1; WiFi.scanDelete(); WiFi.scanNetworks(true); }
int wifi_scan_poll() { int s = s_scanState; if (s >= 0) s_scanState = -2; return s; }
const char* wifi_scan_ssid(int i) { return (i >= 0 && i < s_scanN) ? s_scan[i] : ""; }
