#include "time_manager.h"
#include "config.h"

static bool s_sntp = false;
static bool s_wasValid = false;
static bool s_event = false;

void time_set_tz(const char* tz) { setenv("TZ", tz, 1); tzset(); }
void time_begin(const char* tz) { time_set_tz(tz); }

void time_start_sntp() {
  if (s_sntp) return;
  s_sntp = true;
  configTime(0, 0, SB_NTP_1, SB_NTP_2, SB_NTP_3);   // TZ already set via setenv
}

bool time_valid() {
  time_t t = time(nullptr);
  return t >= (time_t)SB_TIME_VALID_AFTER;
}

time_t time_now() { return time_valid() ? time(nullptr) : 0; }

bool time_local(struct tm* out) {
  if (!time_valid()) return false;
  time_t t = time(nullptr);
  localtime_r(&t, out);
  return true;
}

bool time_take_valid_event() {
  bool v = time_valid();
  if (v && !s_wasValid) { s_wasValid = true; s_event = true; }
  if (s_event) { s_event = false; return true; }
  return false;
}
