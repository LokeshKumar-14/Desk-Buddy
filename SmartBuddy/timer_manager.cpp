#include "timer_manager.h"
#include "settings_manager.h"
#include <esp_timer.h>

static TimerState s_st = TMR_IDLE;
static uint32_t   s_dur = 1500;          // seconds
static int64_t    s_endUs = 0;
static uint32_t   s_remMs = 0;           // valid while paused/idle
static bool       s_doneEvt = false;
static uint32_t   s_ver = 1;

static inline int64_t nowUs() { return esp_timer_get_time(); }

void timer_begin() {
  s_dur = settings_get().timerSec;
  s_remMs = s_dur * 1000UL;
  s_st = TMR_IDLE;
}

TimerState timer_state() { return s_st; }
uint32_t timer_duration_sec() { return s_dur; }
uint32_t timer_version() { return s_ver; }

uint32_t timer_remaining_ms() {
  if (s_st == TMR_RUNNING) {
    int64_t left = s_endUs - nowUs();
    return left > 0 ? (uint32_t)(left / 1000) : 0;
  }
  if (s_st == TMR_DONE) return 0;
  return s_remMs;
}

bool timer_set_duration(uint32_t sec) {
  if (s_st == TMR_RUNNING || s_st == TMR_PAUSED) return false;
  sec = constrain(sec, 10UL, 21600UL);
  s_dur = sec; s_remMs = sec * 1000UL; s_st = TMR_IDLE;
  if (settings_get().timerSec != sec) settings_edit().timerSec = (uint16_t)sec;
  s_ver++;
  return true;
}

void timer_start() {
  if (s_st == TMR_RUNNING) return;
  if (s_st == TMR_IDLE || s_st == TMR_DONE) s_remMs = s_dur * 1000UL;
  s_endUs = nowUs() + (int64_t)s_remMs * 1000;
  s_st = TMR_RUNNING;
  s_ver++;
}

void timer_pause() {
  if (s_st != TMR_RUNNING) return;
  s_remMs = timer_remaining_ms();
  s_st = TMR_PAUSED;
  s_ver++;
}

void timer_reset() {
  s_st = TMR_IDLE;
  s_remMs = s_dur * 1000UL;
  s_doneEvt = false;
  s_ver++;
}

void timer_loop() {
  if (s_st == TMR_RUNNING && nowUs() >= s_endUs) {
    s_st = TMR_DONE; s_remMs = 0; s_doneEvt = true; s_ver++;
  }
}

bool timer_take_done_event() { bool e = s_doneEvt; s_doneEvt = false; return e; }
