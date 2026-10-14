// Countdown timer on the monotonic esp_timer clock (no drift, no rollover).
#pragma once
#include <Arduino.h>

enum TimerState : uint8_t { TMR_IDLE = 0, TMR_RUNNING, TMR_PAUSED, TMR_DONE };

void        timer_begin();
void        timer_loop();
TimerState  timer_state();
uint32_t    timer_remaining_ms();
uint32_t    timer_duration_sec();
bool        timer_set_duration(uint32_t sec);     // only while idle/done; persists as default
void        timer_start();                        // idle/done: start; paused: resume
void        timer_pause();
void        timer_reset();
bool        timer_take_done_event();              // true once when a countdown completes
uint32_t    timer_version();
