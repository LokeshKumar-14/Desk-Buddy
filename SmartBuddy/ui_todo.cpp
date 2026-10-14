#include "ui_todo.h"
#include "ui_common.h"
#include "task_manager.h"
#include "storage_manager.h"
#include "wifi_manager.h"
#include "config.h"

static lv_obj_t* s_list;
static lv_obj_t* s_count;
static lv_obj_t* s_empty;
static uint32_t  s_key = 0;
static int       s_listW = 340;

static void checkCb(lv_event_t* e) { tasks_toggle((uint32_t)(uintptr_t)lv_event_get_user_data(e)); }
static void delCb(lv_event_t* e)   { tasks_delete((uint32_t)(uintptr_t)lv_event_get_user_data(e)); }

lv_obj_t* ui_todo_create(lv_obj_t* parent, int x, int y, int w, int h) {
  lv_obj_t* c = ui_card(parent, x, y, w, h);
  lv_obj_t* t = ui_label(c, &lv_font_montserrat_24, "To-Do");
  lv_obj_align(t, LV_ALIGN_TOP_LEFT, 0, 0);
  s_count = ui_label(c, &lv_font_montserrat_14, "", true);
  lv_obj_align(s_count, LV_ALIGN_TOP_RIGHT, 0, 6);

  s_listW = w - 24;
  s_list = lv_obj_create(c);
  lv_obj_remove_style_all(s_list);
  lv_obj_set_pos(s_list, 0, 40);
  lv_obj_set_size(s_list, s_listW, h - 24 - 40);
  lv_obj_set_flex_flow(s_list, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_scroll_dir(s_list, LV_DIR_VER);
  lv_obj_add_style(s_list, theme_style(ST_SCROLLBAR), LV_PART_SCROLLBAR);
  lv_obj_set_scrollbar_mode(s_list, LV_SCROLLBAR_MODE_AUTO);

  s_empty = ui_label(c, &lv_font_montserrat_16, "", true);
  lv_obj_set_width(s_empty, s_listW);
  lv_label_set_long_mode(s_empty, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(s_empty, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(s_empty, LV_ALIGN_CENTER, 0, 10);
  return c;
}

static void addRow(const Task* t) {
  lv_obj_t* row = lv_obj_create(s_list);
  lv_obj_remove_style_all(row);
  lv_obj_set_size(row, s_listW - 6, 46);
  lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_width(row, 1, 0);
  lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
  lv_obj_set_style_border_color(row, lv_color_hex(theme().text), 0);
  lv_obj_set_style_border_opa(row, 28, 0);

  void* id = (void*)(uintptr_t)t->id;
  lv_obj_t* chk = ui_button(row, 34, 34, t->done ? LV_SYMBOL_OK : "", &lv_font_montserrat_16,
                            t->done ? BTN_PRIMARY : BTN_GLASS, checkCb, id);
  lv_obj_set_style_radius(chk, LV_RADIUS_CIRCLE, 0);
  lv_obj_align(chk, LV_ALIGN_LEFT_MID, 0, 0);

  lv_obj_t* l = lv_label_create(row);
  lv_obj_add_style(l, theme_style(ST_TEXT), 0);
  if (t->done) lv_obj_add_style(l, theme_style(ST_DONE), 0);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_16, 0);
  lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
  lv_obj_set_width(l, s_listW - 6 - 34 - 34 - 24);
  lv_label_set_text(l, t->text);
  lv_obj_align(l, LV_ALIGN_LEFT_MID, 46, 0);

  lv_obj_t* del = ui_button(row, 34, 34, LV_SYMBOL_CLOSE, &lv_font_montserrat_14, BTN_GLASS, delCb, id);
  lv_obj_set_style_radius(del, LV_RADIUS_CIRCLE, 0);
  lv_obj_align(del, LV_ALIGN_RIGHT_MID, 0, 0);
}

void ui_todo_tick() {
  uint32_t key = tasks_version() * 31u + wifi_version() * 7u + (uint32_t)theme_index() * 3u +
                 (storage_ok() ? 1u : 0u) + (tasks_save_ok() ? 2u : 0u);
  if (key == s_key) return;
  s_key = key;

  lv_obj_clean(s_list);
  int n = tasks_count();
  for (int i = 0; i < n; i++) addRow(tasks_at(i));

  char buf[64];
  if (!storage_ok()) snprintf(buf, sizeof(buf), "SD card missing - not saved");
  else if (!tasks_save_ok()) snprintf(buf, sizeof(buf), "Save failed - retrying");
  else snprintf(buf, sizeof(buf), "%d active  |  %d done", tasks_active_count(), tasks_done_count());
  lv_label_set_text(s_count, buf);

  if (n == 0) {
    char ip[20];
    wifi_ip_str(ip, sizeof(ip));
    if (ip[0]) snprintf(buf, sizeof(buf), "No tasks yet.\nAdd them from your phone:\nhttp://%s/", ip);
    else snprintf(buf, sizeof(buf), "No tasks yet.\nConnect Wi-Fi to add tasks\nfrom your phone.");
    lv_label_set_text(s_empty, buf);
    lv_obj_remove_flag(s_empty, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(s_empty, LV_OBJ_FLAG_HIDDEN);
  }
}
