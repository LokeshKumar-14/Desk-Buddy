#include "ui_timer.h"
#include "ui_common.h"
#include "timer_manager.h"

static lv_obj_t *s_time, *s_state, *s_bar, *s_play;
static uint32_t  s_shownSec = 0xFFFFFFFF;
static TimerState s_shownState = (TimerState)255;
static uint32_t  s_shownDur = 0;

static void adjust(int dir) {
  TimerState st = timer_state();
  if (st == TMR_RUNNING || st == TMR_PAUSED) { ui_toast("Reset the timer first"); return; }
  uint32_t d = timer_duration_sec();
  uint32_t step = d >= 120 ? 60 : 10;
  if (dir < 0) d = d > step ? d - step : 10; else d += step;
  timer_set_duration(d);
}
static void minusCb(lv_event_t* e) { (void)e; adjust(-1); }
static void plusCb(lv_event_t* e)  { (void)e; adjust(+1); }
static void playCb(lv_event_t* e) {
  (void)e;
  if (timer_state() == TMR_RUNNING) timer_pause(); else timer_start();
}
static void resetCb(lv_event_t* e) { (void)e; timer_reset(); }
static void dismissCb(void*) { timer_reset(); }

lv_obj_t* ui_timer_create(lv_obj_t* parent, int x, int y, int w, int h) {
  lv_obj_t* c = ui_card(parent, x, y, w, h);
  s_time = ui_label(c, &lv_font_montserrat_40, "25:00");
  lv_obj_set_pos(s_time, 0, -2);
  s_state = ui_label(c, &lv_font_montserrat_14, "Ready", true);
  lv_obj_set_pos(s_state, 2, 48);

  const int bw = 46, gap = 8, inner = w - 24;
  int bx = inner - (4 * bw + 3 * gap);
  ui_button(c, bw, bw, LV_SYMBOL_MINUS, &lv_font_montserrat_16, BTN_GLASS, minusCb);
  lv_obj_set_pos(lv_obj_get_child(c, -1), bx, 0);
  ui_button(c, bw, bw, LV_SYMBOL_PLUS, &lv_font_montserrat_16, BTN_GLASS, plusCb);
  lv_obj_set_pos(lv_obj_get_child(c, -1), bx + (bw + gap), 0);
  s_play = ui_button(c, bw, bw, LV_SYMBOL_PLAY, &lv_font_montserrat_20, BTN_PRIMARY, playCb);
  lv_obj_set_pos(s_play, bx + 2 * (bw + gap), 0);
  ui_button(c, bw, bw, LV_SYMBOL_STOP, &lv_font_montserrat_16, BTN_GLASS, resetCb);
  lv_obj_set_pos(lv_obj_get_child(c, -1), bx + 3 * (bw + gap), 0);

  s_bar = lv_bar_create(c);
  lv_obj_remove_style_all(s_bar);
  lv_obj_add_style(s_bar, theme_style(ST_BAR), LV_PART_MAIN);
  lv_obj_add_style(s_bar, theme_style(ST_BAR_IND), LV_PART_INDICATOR);
  lv_bar_set_range(s_bar, 0, 1000);
  lv_obj_set_size(s_bar, inner, 8);
  lv_obj_set_pos(s_bar, 0, h - 24 - 8);
  lv_bar_set_value(s_bar, 0, LV_ANIM_OFF);
  return c;
}

void ui_timer_tick() {
  if (timer_take_done_event())
    ui_confirm("Timer finished", "Your countdown has completed.", "Dismiss", nullptr, BTN_PRIMARY, dismissCb, nullptr);

  TimerState st = timer_state();
  uint32_t remMs = timer_remaining_ms();
  uint32_t sec = (remMs + 999) / 1000;
  uint32_t dur = timer_duration_sec();
  if (sec == s_shownSec && st == s_shownState && dur == s_shownDur) return;

  char buf[24];
  if (sec >= 3600) snprintf(buf, sizeof(buf), "%u:%02u:%02u", (unsigned)(sec / 3600), (unsigned)((sec / 60) % 60), (unsigned)(sec % 60));
  else snprintf(buf, sizeof(buf), "%02u:%02u", (unsigned)(sec / 60), (unsigned)(sec % 60));
  lv_label_set_text(s_time, buf);

  if (st != s_shownState) {
    const char* txt = st == TMR_RUNNING ? "Running" : st == TMR_PAUSED ? "Paused" : st == TMR_DONE ? "Finished" : "Ready";
    lv_label_set_text(s_state, txt);
    ui_button_set_text(s_play, st == TMR_RUNNING ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
  }
  int32_t v = 0;
  if (dur > 0 && st != TMR_IDLE) {
    uint32_t total = dur * 1000UL;
    uint32_t el = total > remMs ? total - remMs : 0;
    v = (int32_t)((uint64_t)el * 1000 / total);
  }
  lv_bar_set_value(s_bar, v, LV_ANIM_OFF);
  s_shownSec = sec; s_shownState = st; s_shownDur = dur;
}
