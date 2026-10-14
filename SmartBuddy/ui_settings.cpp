#include "ui_settings.h"
#include "ui_common.h"
#include "settings_manager.h"
#include "theme_manager.h"
#include "wallpaper_manager.h"
#include "display_setup.h"
#include "timer_manager.h"
#include "time_manager.h"
#include "wifi_manager.h"
#include "storage_manager.h"
#include "weather_manager.h"
#include "task_manager.h"
#include "config.h"
#include <esp_system.h>

// ---------- layout constants ----------
static const int PAD   = 14;                 // card inner padding
static const int CARD_W = 470;
static const int W     = CARD_W - 2 * PAD;   // usable content width (442)
static const int SW_SZ = 44;                 // swatch size
static const int SW_GAP = 10;
static const int SW_COLS = 8;

// Right-aligned stepper block: [btn 46][4][label 52][4][btn 46] = 152
static const int STEP_X   = W - 152;
static const int STEP_LBL = STEP_X + 50;
static const int STEP_PLUS = W - 46;

// ---------- color palette ----------
static const uint32_t PALETTE[16] = {
  0x000000, 0x1F2937, 0x0B1E3F, 0x1E3A8A,
  0x2563EB, 0x0891B2, 0x0F766E, 0x15803D,
  0x65A30D, 0xCA8A04, 0xEA580C, 0xDC2626,
  0xDB2777, 0x7C3AED, 0x6B7280, 0xF3F4F6
};

static lv_obj_t *s_ov, *s_status;
static lv_obj_t *s_btnDark, *s_btnLight, *s_tzLbl, *s_ssLbl, *s_tmLbl, *s_brLbl, *s_colLbl;
static lv_obj_t *s_colPrev;
static lv_obj_t *s_sw[16];
static bool s_open = false;
static uint32_t s_lastStatus = 0;

static void refreshControls() {
  const SbSettings& s = settings_get();
  ui_button_set_kind(s_btnDark, s.theme == 0 ? BTN_PRIMARY : BTN_GLASS);
  ui_button_set_kind(s_btnLight, s.theme == 1 ? BTN_PRIMARY : BTN_GLASS);
  char b[64];
  int i = settings_tz_find(s.tz);
  lv_label_set_text(s_tzLbl, i >= 0 ? settings_tz(i).label : s.tz);
  snprintf(b, sizeof(b), "%u s", (unsigned)s.slideshowSec);
  lv_label_set_text(s_ssLbl, b);
  snprintf(b, sizeof(b), "%u min", (unsigned)(timer_duration_sec() / 60));
  lv_label_set_text(s_tmLbl, b);
  snprintf(b, sizeof(b), "%u%%", (unsigned)s.brightness);
  lv_label_set_text(s_brLbl, b);

  // color preview + hex
  uint32_t col = s.bgColor & 0xFFFFFF;
  lv_obj_set_style_bg_color(s_colPrev, lv_color_hex(col), 0);
  if (s.bgColorOnly) snprintf(b, sizeof(b), "#%06lX", (unsigned long)col);
  else               snprintf(b, sizeof(b), "Photo wallpaper");
  lv_label_set_text(s_colLbl, b);

  // highlight selected swatch
  for (int k = 0; k < 16; k++) {
    bool sel = s.bgColorOnly && col == PALETTE[k];
    lv_obj_set_style_border_width(s_sw[k], sel ? 4 : 1, 0);
    lv_obj_set_style_border_color(s_sw[k], sel ? lv_color_hex(0x2F80ED) : lv_color_hex(0x808080), 0);
    lv_obj_set_style_border_opa(s_sw[k], sel ? LV_OPA_COVER : LV_OPA_50, 0);
  }
}

static void setTheme(int t) {
  if (settings_get().theme == t) return;
  settings_edit().theme = (uint8_t)t;
  theme_apply((uint8_t)t);
  wallpaper_on_theme_changed();
  refreshControls();
}
static void darkCb(lv_event_t* e)  { (void)e; setTheme(0); }
static void lightCb(lv_event_t* e) { (void)e; setTheme(1); }

static void tzStep(int d) {
  int n = settings_tz_count();
  int i = settings_tz_find(settings_get().tz);
  i = (i < 0) ? 0 : ((i + d) % n + n) % n;
  strlcpy(settings_edit().tz, settings_tz(i).posix, sizeof(settings_get().tz));
  time_set_tz(settings_get().tz);
  refreshControls();
}
static void tzPrev(lv_event_t* e) { (void)e; tzStep(-1); }
static void tzNext(lv_event_t* e) { (void)e; tzStep(+1); }

static void ssStep(int d) {
  uint32_t v = settings_get().slideshowSec;
  uint32_t step = v < 10 ? 1 : (v < 60 ? 5 : 10);
  int32_t nv = (int32_t)v + d * (int32_t)step;
  settings_edit().slideshowSec = (uint16_t)constrain(nv, 3, 300);
  refreshControls();
}
static void ssMinus(lv_event_t* e) { (void)e; ssStep(-1); }
static void ssPlus(lv_event_t* e)  { (void)e; ssStep(+1); }

static void tmStep(int d) {
  TimerState st = timer_state();
  if (st == TMR_RUNNING || st == TMR_PAUSED) { ui_toast("Reset the timer first"); return; }
  int32_t nv = (int32_t)timer_duration_sec() + d * 60;
  timer_set_duration((uint32_t)constrain(nv, 60, 21600));
  refreshControls();
}
static void tmMinus(lv_event_t* e) { (void)e; tmStep(-1); }
static void tmPlus(lv_event_t* e)  { (void)e; tmStep(+1); }

static void brightnessStep(int d) {
  int v = (int)settings_get().brightness + d * 5;
  v = constrain(v, 5, 100);
  settings_edit().brightness = (uint8_t)v;
  display_set_brightness((uint8_t)v);
  refreshControls();
}
static void brMinus(lv_event_t* e) { (void)e; brightnessStep(-1); }
static void brPlus(lv_event_t* e)  { (void)e; brightnessStep(+1); }

// Tap on a palette swatch -> set background color immediately
static void swatchCb(lv_event_t* e) {
  uintptr_t idx = (uintptr_t)lv_event_get_user_data(e);
  if (idx >= 16) return;
  SbSettings& s = settings_edit();
  s.bgColor = PALETTE[idx];
  s.bgColorOnly = true;
  s.bg[0] = 0;
  wallpaper_request_color();
  refreshControls();
  ui_toast("Background color set");
}

static void doReboot(void*) {
  settings_flush();
  delay(300);
  ESP.restart();
}
static void rebootCb(lv_event_t* e) {
  (void)e;
  ui_confirm("Reboot device?", "Smart Buddy will restart. Tasks and settings are kept.", "Reboot", "Cancel", BTN_DANGER, doReboot, nullptr);
}

static void onClose() { s_open = false; }

// ---------- helpers ----------
static lv_obj_t* rowLabel(lv_obj_t* card, const char* txt, int y) {
  lv_obj_t* l = ui_label(card, &lv_font_montserrat_16, txt);
  lv_obj_set_pos(l, 0, y + 10);
  return l;
}
static lv_obj_t* small(lv_obj_t* card, int x, int y, const char* t, lv_event_cb_t cb) {
  lv_obj_t* b = ui_button(card, 46, 38, t, &lv_font_montserrat_16, BTN_GLASS, cb);
  lv_obj_set_pos(b, x, y);
  return b;
}
// [-] value [+] block, right aligned. Returns the value label.
static lv_obj_t* stepper(lv_obj_t* card, int y, lv_event_cb_t minusCb, lv_event_cb_t plusCb) {
  small(card, STEP_X, y, LV_SYMBOL_MINUS, minusCb);
  small(card, STEP_PLUS, y, LV_SYMBOL_PLUS, plusCb);
  lv_obj_t* l = ui_label(card, &lv_font_montserrat_16, "");
  lv_obj_set_width(l, 52);
  lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_pos(l, STEP_LBL, y + 10);
  return l;
}

lv_obj_t* ui_settings_create(lv_obj_t* parent) {
  s_ov = ui_overlay_create(parent, "Settings", onClose);
  lv_obj_t* c = ui_card(s_ov, 12, 68, CARD_W, 400);

  // ----- vertical scrolling -----
  lv_obj_set_style_pad_all(c, PAD, 0);
  lv_obj_add_flag(c, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(c, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(c, LV_SCROLLBAR_MODE_AUTO);

  // ----- Theme (y=0) -----
  rowLabel(c, "Theme", 0);
  s_btnDark = ui_button(c, 100, 38, "Dark", &lv_font_montserrat_14, BTN_PRIMARY, darkCb);
  lv_obj_set_pos(s_btnDark, W - 208, 0);
  s_btnLight = ui_button(c, 100, 38, "Light", &lv_font_montserrat_14, BTN_GLASS, lightCb);
  lv_obj_set_pos(s_btnLight, W - 100, 0);

  // ----- Brightness (y=52) -----
  rowLabel(c, "Brightness", 52);
  s_brLbl = stepper(c, 52, brMinus, brPlus);

  // ----- Background color (y=104) -----
  rowLabel(c, "Background", 104);
  s_colPrev = lv_obj_create(c);
  lv_obj_set_size(s_colPrev, 38, 38);
  lv_obj_set_pos(s_colPrev, W - 38, 104);
  lv_obj_set_style_radius(s_colPrev, 10, 0);
  lv_obj_set_style_bg_opa(s_colPrev, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(s_colPrev, 2, 0);
  lv_obj_set_style_border_color(s_colPrev, lv_color_hex(0x808080), 0);
  lv_obj_set_style_pad_all(s_colPrev, 0, 0);

  s_colLbl = ui_label(c, &lv_font_montserrat_14, "", true);
  lv_obj_set_width(s_colLbl, 190);
  lv_label_set_long_mode(s_colLbl, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_align(s_colLbl, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_pos(s_colLbl, W - 38 - 8 - 190, 104 + 11);

  // palette grid: 8 x 2
  for (int k = 0; k < 16; k++) {
    int col = k % SW_COLS, row = k / SW_COLS;
    lv_obj_t* sw = lv_obj_create(c);
    lv_obj_set_size(sw, SW_SZ, SW_SZ);
    lv_obj_set_pos(sw, col * (SW_SZ + SW_GAP), 152 + row * (SW_SZ + SW_GAP));
    lv_obj_set_style_radius(sw, 12, 0);
    lv_obj_set_style_bg_color(sw, lv_color_hex(PALETTE[k]), 0);
    lv_obj_set_style_bg_opa(sw, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(sw, 0, 0);
    lv_obj_add_flag(sw, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(sw, swatchCb, LV_EVENT_CLICKED, (void*)(uintptr_t)k);
    s_sw[k] = sw;
  }

  // ----- Time zone (y=266) -----
  rowLabel(c, "Time zone", 266);
  small(c, W - 240, 266, LV_SYMBOL_LEFT, tzPrev);
  s_tzLbl = ui_label(c, &lv_font_montserrat_14, "", true);
  lv_obj_set_width(s_tzLbl, 140);
  lv_label_set_long_mode(s_tzLbl, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_align(s_tzLbl, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_pos(s_tzLbl, W - 240 + 50, 266 + 12);
  small(c, W - 46, 266, LV_SYMBOL_RIGHT, tzNext);

  // ----- Slideshow (y=318) -----
  rowLabel(c, "Slideshow", 318);
  s_ssLbl = stepper(c, 318, ssMinus, ssPlus);

  // ----- Default timer (y=370) -----
  rowLabel(c, "Default timer", 370);
  s_tmLbl = stepper(c, 370, tmMinus, tmPlus);

  // ----- Reboot (y=426) -----
  lv_obj_t* rb = ui_button(c, 140, 38, "Reboot", &lv_font_montserrat_14, BTN_DANGER, rebootCb);
  lv_obj_set_pos(rb, 0, 426);

  // ----- Status card -----
  lv_obj_t* sc = ui_card(s_ov, 492, 68, 296, 400);
  ui_label(sc, &lv_font_montserrat_20, "Device status");
  s_status = ui_label(sc, &lv_font_montserrat_14, "", true);
  lv_obj_set_width(s_status, 272);
  lv_obj_set_pos(s_status, 0, 34);
  return s_ov;
}

void ui_settings_open() {
  refreshControls();
  s_open = true;
  s_lastStatus = 0;
  ui_nav_open(s_ov);
}

void ui_settings_tick() {
  if (!s_open) return;
  uint32_t now = millis();
  if (s_lastStatus && now - s_lastStatus < 1000) return;
  s_lastStatus = now ? now : 1;
  char ip[20], buf[420];
  wifi_ip_str(ip, sizeof(ip));
  const char* wst = wifi_state() == SBW_CONNECTED ? "connected" : wifi_state() == SBW_CONNECTING ? "connecting" :
                    wifi_state() == SBW_FAILED ? "failed" : "not set up";
  const char* sd = storage_state() == SD_ST_OK ? "OK" : storage_state() == SD_ST_MISSING ? "no card" : "error";
  uint32_t up = now / 1000;
  WeatherData wd; WeatherStatus ws; bool stale;
  weather_get(&wd, &ws, &stale);
  snprintf(buf, sizeof(buf),
           "Wi-Fi: %s\nNetwork: %s\nIP: %s\nInternet: %s\nClock: %s\nSD card: %s\n"
           "SD size: %u MB, free %u MB\nTasks: %d\nFree heap: %u KB\nFree PSRAM: %u KB\n"
           "Uptime: %uh %02um %02us\nBuild: %s",
           wst, wifi_ssid(), ip[0] ? ip : "-", wifi_internet_ok() ? "yes" : "no",
           time_valid() ? "synced" : "not synced", sd,
           (unsigned)(storage_total_bytes() / 1048576ULL), (unsigned)(storage_free_bytes() / 1048576ULL),
           tasks_count(), (unsigned)(ESP.getFreeHeap() / 1024), (unsigned)(ESP.getFreePsram() / 1024),
           (unsigned)(up / 3600), (unsigned)((up / 60) % 60), (unsigned)(up % 60), __DATE__);
  lv_label_set_text(s_status, buf);
}