// Non-blocking Wi-Fi station manager (state machine, backoff retries).
// The password is never printed and never returned by any getter.
#pragma once
#include <Arduino.h>

enum WifiMgrState : uint8_t { SBW_NO_CREDS = 0, SBW_CONNECTING, SBW_CONNECTED, SBW_FAILED };

void          wifi_begin();
void          wifi_loop();
WifiMgrState  wifi_state();
bool          wifi_connected();
const char*   wifi_ssid();                       // configured SSID
void          wifi_ip_str(char* buf, size_t n);  // "" when not connected
int           wifi_rssi();
bool          wifi_set_credentials(const char* ssid, const char* pass);  // saves to NVS + reconnects
void          wifi_reconnect();
void          wifi_set_enabled(bool on);         // radio on/off at runtime (debug / power)
bool          wifi_enabled();
bool          wifi_take_connected_event();       // true once per (re)connection
uint32_t      wifi_version();
uint32_t      wifi_retry_in_sec();               // when FAILED

// Internet reachability (set by the network task)
void          wifi_set_internet(bool ok);
bool          wifi_internet_ok();

// Async scan
void          wifi_scan_start();
int           wifi_scan_poll();                  // -2 idle, -1 running, >=0 result count
const char*   wifi_scan_ssid(int i);
