#include "ui_daily.h"
#include "ui_common.h"
#include "daily_manager.h"

static lv_obj_t *s_ov, *s_list;
static bool s_open = false;
static uint32_t s_seen = 0;

static void onClose() { s_open = false; }

static void toggleTaskCb(lv_event_t* e) {
  daily_toggle_task((uint32_t)(uintptr_t)lv_event_get_user_data(e));
}

static void toggleSubCb(lv_event_t* e) {
  uint32_t packed = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
  daily_toggle_sub(packed >> 16, packed & 0xFFFF);
}

static void rebuild() {
  lv_obj_clean(s_list);
  for (int i = 0; i < daily_count(); i++) {
    const DailyTask* t = daily_at(i);
    lv_obj_t* card = ui_card(s_list, 0, 0, 700, 56 + t->subCount * 34);
    lv_obj_set_width(card, 700);

    lv_obj_t* chk = ui_button(card, 42, 34, t->done ? LV_SYMBOL_OK : "", &lv_font_montserrat_16,
                              t->done ? BTN_PRIMARY : BTN_GLASS, toggleTaskCb, (void*)(uintptr_t)t->id);
    lv_obj_set_pos(chk, 0, 0);

    // title + progress (done/total)
    char tb[96];
    if (t->subCount > 0) {
      int d = 0;
      for (uint8_t j = 0; j < t->subCount; j++) if (t->subs[j].done) d++;
      snprintf(tb, sizeof(tb), "%s  (%d/%d)", t->text, d, (int)t->subCount);
    } else {
      snprintf(tb, sizeof(tb), "%s", t->text);
    }
    lv_obj_t* title = ui_label(card, &lv_font_montserrat_18, tb);
    lv_obj_set_width(title, 620);
    lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
    lv_obj_set_pos(title, 52, 6);
    if (t->done) lv_obj_set_style_text_decor(title, LV_TEXT_DECOR_STRIKETHROUGH, 0);

    for (uint8_t j = 0; j < t->subCount; j++) {
      const DailySubtask& st = t->subs[j];
      uint32_t key = (t->id << 16) | (st.id & 0xFFFF);
      lv_obj_t* b = ui_button(card, 34, 28, st.done ? LV_SYMBOL_OK : "", &lv_font_montserrat_14,
                              st.done ? BTN_PRIMARY : BTN_GLASS, toggleSubCb, (void*)(uintptr_t)key);
      lv_obj_set_pos(b, 52, 44 + j * 32);
      lv_obj_t* l = ui_label(card, &lv_font_montserrat_14, st.text, true);
      lv_obj_set_width(l, 570);
      lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
      lv_obj_set_pos(l, 94, 49 + j * 32);
      if (st.done) lv_obj_set_style_text_decor(l, LV_TEXT_DECOR_STRIKETHROUGH, 0);
    }
  }
  if (daily_count() == 0) {
    lv_obj_t* empty = ui_label(s_list, &lv_font_montserrat_20, "Daily tasks can be added from the phone web page.", true);
    lv_obj_set_pos(empty, 0, 0);
  }
  s_seen = daily_version();
}

lv_obj_t* ui_daily_create(lv_obj_t* parent) {
  s_ov = ui_overlay_create(parent, "Daily", onClose);
  lv_obj_t* hint = ui_label(s_ov, &lv_font_montserrat_14,
                            "All subtasks done = task done. Resets automatically next day.", true);
  lv_obj_set_pos(hint, 82, 52);
  s_list = lv_obj_create(s_ov);
  lv_obj_remove_style_all(s_list);
  lv_obj_set_pos(s_list, 48, 84);
  lv_obj_set_size(s_list, 720, 370);
  lv_obj_set_flex_flow(s_list, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(s_list, 10, 0);
  lv_obj_add_flag(s_list, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(s_list, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(s_list, LV_SCROLLBAR_MODE_AUTO);
  rebuild();
  return s_ov;
}

void ui_daily_open() {
  s_open = true;
  rebuild();
  ui_nav_open(s_ov);
}

void ui_daily_tick() {
  if (s_open && s_seen != daily_version()) rebuild();
}