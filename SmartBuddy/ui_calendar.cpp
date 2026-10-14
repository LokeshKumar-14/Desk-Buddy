#include "ui_calendar.h"
#include "ui_common.h"
#include "time_manager.h"
#include "config.h"

static lv_obj_t* s_title;
static lv_obj_t* s_hdr[7];
static lv_obj_t* s_cell[42];
static int  s_vy = 0, s_vm = 0;          // viewed month (1..12)
static int  s_curY = 0, s_curM = 0, s_curD = 0;
static bool s_haveView = false;
static uint32_t s_key = 0;

static const char* kMonths[12] = { "January", "February", "March", "April", "May", "June", "July",
                                   "August", "September", "October", "November", "December" };
#if SB_WEEK_STARTS_MONDAY
static const char* kDays[7] = { "Mo", "Tu", "We", "Th", "Fr", "Sa", "Su" };
#else
static const char* kDays[7] = { "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa" };
#endif

static int daysIn(int y, int m) {
  static const uint8_t d[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) return 29;
  return d[m - 1];
}
static int weekdaySun0(int y, int m, int d) {          // Sakamoto, 0 = Sunday
  static const int t[12] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
  if (m < 3) y--;
  return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

static void shift(int delta) {
  if (!s_haveView) return;
  s_vm += delta;
  if (s_vm < 1) { s_vm = 12; s_vy--; }
  if (s_vm > 12) { s_vm = 1; s_vy++; }
  if (s_vy < 2025) { s_vy = 2025; s_vm = 1; }
  if (s_vy > 2100) { s_vy = 2100; s_vm = 12; }
  s_key = 0;
}
static void prevCb(lv_event_t* e) { (void)e; shift(-1); }
static void nextCb(lv_event_t* e) { (void)e; shift(+1); }
static void homeCb(lv_event_t* e) { (void)e; if (s_haveView) { s_vy = s_curY; s_vm = s_curM; s_key = 0; } }

lv_obj_t* ui_calendar_create(lv_obj_t* parent, int x, int y, int w, int h) {
  lv_obj_t* c = ui_card(parent, x, y, w, h);
  lv_obj_set_style_pad_all(c, 8, 0);
  const int colW = (w - 16) / 7;

  lv_obj_t* prev = ui_button(c, 40, 26, LV_SYMBOL_LEFT, &lv_font_montserrat_14, BTN_GLASS, prevCb);
  lv_obj_set_pos(prev, 0, 0);
  lv_obj_t* next = ui_button(c, 40, 26, LV_SYMBOL_RIGHT, &lv_font_montserrat_14, BTN_GLASS, nextCb);
  lv_obj_set_pos(next, w - 16 - 40, 0);
  s_title = ui_label(c, &lv_font_montserrat_16, "");
  lv_obj_set_width(s_title, w - 16 - 100);
  lv_obj_set_style_text_align(s_title, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_pos(s_title, 50, 4);
  lv_obj_add_flag(s_title, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(s_title, homeCb, LV_EVENT_CLICKED, nullptr);

  for (int i = 0; i < 7; i++) {
    s_hdr[i] = ui_label(c, &lv_font_montserrat_12, kDays[i], true);
    lv_obj_set_width(s_hdr[i], colW);
    lv_obj_set_style_text_align(s_hdr[i], LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(s_hdr[i], i * colW, 31);
  }
  for (int r = 0; r < 6; r++) {
    for (int k = 0; k < 7; k++) {
      lv_obj_t* l = ui_label(c, &lv_font_montserrat_14, "");
      lv_obj_set_size(l, 32, 19);
      lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_set_pos(l, k * colW + (colW - 32) / 2, 47 + r * 19);
      s_cell[r * 7 + k] = l;
    }
  }
  return c;
}

void ui_calendar_tick() {
  struct tm lt;
  if (!time_local(&lt)) {
    if (s_haveView) { s_haveView = false; s_key = 0; }
    if (s_key != 1) {
      s_key = 1;
      lv_label_set_text(s_title, "Waiting for time sync");
      for (int i = 0; i < 42; i++) {
        lv_label_set_text(s_cell[i], "");
        lv_obj_remove_style(s_cell[i], theme_style(ST_TODAY), 0);
      }
    }
    return;
  }
  s_curY = lt.tm_year + 1900; s_curM = lt.tm_mon + 1; s_curD = lt.tm_mday;
  if (!s_haveView) { s_haveView = true; s_vy = s_curY; s_vm = s_curM; s_key = 0; }
  uint32_t key = (uint32_t)(s_vy * 100 + s_vm) * 100000u + (uint32_t)(s_curY * 10000 + s_curM * 100 + s_curD);
  if (key == s_key) return;
  s_key = key;

  char buf[40];
  snprintf(buf, sizeof(buf), "%s %d", kMonths[s_vm - 1], s_vy);
  lv_label_set_text(s_title, buf);

  int first = weekdaySun0(s_vy, s_vm, 1);
#if SB_WEEK_STARTS_MONDAY
  first = (first + 6) % 7;
#endif
  int n = daysIn(s_vy, s_vm);
  for (int i = 0; i < 42; i++) {
    int day = i - first + 1;
    lv_obj_t* l = s_cell[i];
    lv_obj_remove_style(l, theme_style(ST_TODAY), 0);
    if (day >= 1 && day <= n) {
      snprintf(buf, sizeof(buf), "%d", day);
      lv_label_set_text(l, buf);
      if (s_vy == s_curY && s_vm == s_curM && day == s_curD) lv_obj_add_style(l, theme_style(ST_TODAY), 0);
    } else {
      lv_label_set_text(l, "");
    }
  }
}
