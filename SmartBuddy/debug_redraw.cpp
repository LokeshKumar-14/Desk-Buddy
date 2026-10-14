#include "debug_redraw.h"
#include "wifi_manager.h"
#include <esp_heap_caps.h>

#ifdef SB_DEBUG_REDRAW
#define DBG_MAX_DEPTH   6
#define DBG_MAX_ENTRIES 24
#define DBG_REPORT_MS   5000
#define DBG_MAX_LINES   30        // max [REDRAW] lines per report window (anti-flood)

static const char* s_stack[DBG_MAX_DEPTH];
static int         s_depth = 0;

struct Entry { char path[96]; uint32_t n; int16_t x1, y1, x2, y2; };
static Entry    s_e[DBG_MAX_ENTRIES];
static int      s_en = 0;
static uint32_t s_inv = 0, s_refr = 0, s_refrMs = 0, s_t0 = 0, s_lastReport = 0, s_lines = 0, s_other = 0;

void dbg_push(const char* n) { if (s_depth < DBG_MAX_DEPTH) s_stack[s_depth] = n; s_depth++; }
void dbg_pop() { if (s_depth > 0) s_depth--; }

static void buildPath(char* out, size_t n) {
  out[0] = 0;
  int d = s_depth < DBG_MAX_DEPTH ? s_depth : DBG_MAX_DEPTH;
  if (d == 0) { strlcpy(out, "(no scope: setup / unknown)", n); return; }
  for (int i = 0; i < d; i++) {
    if (i) strlcat(out, ">", n);
    strlcat(out, s_stack[i], n);
  }
}

static void invCb(lv_event_t* e) {
  const lv_area_t* a = (const lv_area_t*)lv_event_get_param(e);
  if (!a) return;
  char p[96];
  buildPath(p, sizeof(p));
  s_inv++;
  int idx = -1;
  for (int i = 0; i < s_en; i++) if (!strcmp(s_e[i].path, p)) { idx = i; break; }
  if (idx < 0 && s_en < DBG_MAX_ENTRIES) {
    idx = s_en++;
    strlcpy(s_e[idx].path, p, sizeof(s_e[idx].path));
    s_e[idx].n = 0;
    s_e[idx].x1 = a->x1; s_e[idx].y1 = a->y1; s_e[idx].x2 = a->x2; s_e[idx].y2 = a->y2;
  }
  if (idx >= 0) s_e[idx].n++; else s_other++;
  if (s_lines < DBG_MAX_LINES) {
    s_lines++;
    Serial.printf("[REDRAW] %lu ms  %s  area x%d..%d y%d..%d (%dx%d)\n", (unsigned long)millis(), p,
                  (int)a->x1, (int)a->x2, (int)a->y1, (int)a->y2,
                  (int)(a->x2 - a->x1 + 1), (int)(a->y2 - a->y1 + 1));
  }
}

static void refrStartCb(lv_event_t*) { s_t0 = millis(); }
static void refrReadyCb(lv_event_t*) { s_refr++; s_refrMs += millis() - s_t0; }

// ---------------------------------------------------------------------------
// Serial console (115200, send ONE letter, no newline needed):
//   ?  help + current state        w  Wi-Fi radio OFF/ON
//   l  pause/resume LVGL           u  pause/resume UI updates
//   s  pause/resume SD polling     a  resume all paused parts
//   c  heap integrity check (memory corruption?)
// Use it to find what makes the screen shift: send 'w' and see if the shifting stops.
// ---------------------------------------------------------------------------
static bool s_skipL = false, s_skipU = false, s_skipS = false;

bool dbg_skip(char what) {
  return what == 'l' ? s_skipL : what == 'u' ? s_skipU : what == 's' ? s_skipS : false;
}

static void printState() {
  Serial.printf("[DBG] state: wifi=%s lvgl=%s ui=%s sd=%s | PCLK=%lu Hz NO_WIFI=%d NO_SD=%d NO_TOUCH=%d WIFI_SLEEP=%d UPSTREAM_DIRECT=%d\n",
                wifi_enabled() ? "ON" : "OFF", s_skipL ? "PAUSED" : "run", s_skipU ? "PAUSED" : "run",
                s_skipS ? "PAUSED" : "run", (unsigned long)SB_PCLK_HZ, SB_DIAG_NO_WIFI, SB_DIAG_NO_SD,
                SB_DIAG_NO_TOUCH, SB_WIFI_SLEEP, SB_UPSTREAM_DIRECT);
  Serial.printf("[DBG] heap: internal free %u (min %u), PSRAM free %u (largest block %u)\n",
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL),
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
                (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
}

void dbg_console() {
  while (Serial.available()) {
    int c = Serial.read();
    switch (c) {
      case 'w': wifi_set_enabled(!wifi_enabled()); break;
      case 'l': s_skipL = !s_skipL; break;
      case 'u': s_skipU = !s_skipU; break;
      case 's': s_skipS = !s_skipS; break;
      case 'c': Serial.printf("[DBG] heap integrity: %s\n", heap_caps_check_integrity_all(true) ? "OK" : "CORRUPTED!"); break;
      case 'a': s_skipL = false; s_skipU = false; s_skipS = false; if (!wifi_enabled()) wifi_set_enabled(true); break;
      case '?': Serial.println("[DBG] keys: w=wifi on/off  l=pause/resume LVGL  u=pause/resume UI  s=pause/resume SD  a=resume all  c=heap check"); break;
      default: continue;
    }
    printState();
  }
}

void dbg_begin() {
  lv_display_t* d = lv_display_get_default();
  if (!d) return;
  lv_display_add_event_cb(d, invCb, LV_EVENT_INVALIDATE_AREA, nullptr);
  lv_display_add_event_cb(d, refrStartCb, LV_EVENT_REFR_START, nullptr);
  lv_display_add_event_cb(d, refrReadyCb, LV_EVENT_REFR_READY, nullptr);
  s_lastReport = millis();
  Serial.println("[DBG] redraw debugger active (SB_DEBUG_REDRAW). Send ? in Serial Monitor for test keys.");
  printState();
}

void dbg_report() {
  uint32_t now = millis();
  if (now - s_lastReport < DBG_REPORT_MS) return;
  s_lastReport = now;
  Serial.printf("[DBG] heap: internal free %u (min %u), PSRAM free %u, wifi=%s\n", (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL), (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL), (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM), wifi_enabled() ? "ON" : "OFF");
  if (s_inv == 0) {
    Serial.println("[DBG] ---- last 5 s: NO redraw requests (screen idle - good) ----");
  } else {
    Serial.printf("[DBG] ---- last 5 s: %lu redraw requests, %lu frames rendered, %lu ms render time ----\n",
                  (unsigned long)s_inv, (unsigned long)s_refr, (unsigned long)s_refrMs);
    for (int i = 0; i < s_en; i++) {
      Serial.printf("[DBG]   %4lu x  %s%s\n", (unsigned long)s_e[i].n, s_e[i].path,
                    s_e[i].n >= 4 ? "   <-- REPEATING: this is a likely culprit" : "");
    }
    if (s_other) Serial.printf("[DBG]   %lu more from other places\n", (unsigned long)s_other);
  }
  s_en = 0; s_inv = 0; s_refr = 0; s_refrMs = 0; s_lines = 0; s_other = 0;
}
#endif
