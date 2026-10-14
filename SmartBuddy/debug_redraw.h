// Redraw debugger: tells you WHICH function / widget causes LVGL to redraw the screen.
// Enable with  #define SB_DEBUG_REDRAW  in config.h (on by default while you debug).
// Serial Monitor (115200) then shows lines like
//   [REDRAW] 12034 ms  ui_dashboard_tick>updateWeather>s_wxFoot  area x70..360 y52..70
// and every 5 s a summary of who requested redraws.
#pragma once
#include <lvgl.h>
#include "config.h"

#ifdef SB_DEBUG_REDRAW
void dbg_begin();                       // hook LVGL display events (call after display_begin)
void dbg_report();                      // call every loop; prints a summary every 5 s
void dbg_console();                     // Serial commands: ? w l u s c  (see debug_redraw.cpp)
bool dbg_skip(char what);               // true while that part is paused by a Serial command
void dbg_push(const char* name);
void dbg_pop();
struct DbgScope {
  explicit DbgScope(const char* n) { dbg_push(n); }
  ~DbgScope() { dbg_pop(); }
};
#define SB_CONCAT2(a, b) a##b
#define SB_CONCAT(a, b) SB_CONCAT2(a, b)
#define SB_SCOPE(name) DbgScope SB_CONCAT(_sb_scope_, __LINE__)(name)
#else
#define dbg_begin() ((void)0)
#define dbg_report() ((void)0)
#define dbg_console() ((void)0)
#define dbg_skip(c) false
#define SB_SCOPE(name) ((void)0)
#endif
