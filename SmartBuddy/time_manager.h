// Wall-clock time: NTP + POSIX timezone. Never fakes a time.
#pragma once
#include <Arduino.h>
#include <time.h>

void     time_begin(const char* tz);        // set TZ (no network needed)
void     time_start_sntp();                 // call once Wi-Fi has an IP
void     time_set_tz(const char* tz);
bool     time_valid();                      // true once clock has been synced
time_t   time_now();                        // epoch seconds (0 if invalid)
bool     time_local(struct tm* out);        // false if not synced
bool     time_take_valid_event();           // true exactly once when sync first completes
