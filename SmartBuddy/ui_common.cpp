#include "ui_common.h"
#include "config.h"
#define SB_SCREEN_W_UI SB_SCREEN_W
#define SB_SCREEN_H_UI SB_SCREEN_H

// ---------------------------------------------------------------- basics
lv_obj_t* ui_card(lv_obj_t* parent, int x, int y, int w, int h) {
  lv_obj_t* c = lv_obj_create(parent);
  lv_obj_remove_style_all(c);
  lv_obj_set_pos(c, x, y);
  lv_obj_set_size(c, w, h);
  lv_obj_add_style(c, theme_style(ST_CARD), 0);
  lv_obj_remove_flag(c, LV_OBJ_FLAG_SCROLLABLE);
  return c;
}

lv_obj_t* ui_label(lv_obj_t* parent, const lv_font_t* font, const char* text, bool dim) {
  lv_obj_t* l = lv_label_create(parent);
  lv_obj_add_style(l, theme_style(dim ? ST_DIM : ST_TEXT), 0);
  lv_obj_set_style_text_font(l, font, 0);
  lv_label_set_text(l, text);
  return l;
}

static void applyKind(lv_obj_t* b, BtnKind k) {
  lv_obj_remove_style_all(b);
  ThemeStyle base = k == BTN_PRIMARY ? ST_PRIMARY : (k == BTN_DANGER ? ST_DANGER : ST_BTN);
  ThemeStyle pr = k == BTN_PRIMARY ? ST_PRIMARY_PR : (k == BTN_DANGER ? ST_DANGER_PR : ST_BTN_PR);
  lv_obj_add_style(b, theme_style(base), 0);
  lv_obj_add_style(b, theme_style(pr), LV_STATE_PRESSED);
  lv_obj_remove_flag(b, LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t* ui_button(lv_obj_t* parent, int w, int h, const char* text, const lv_font_t* font,
                    BtnKind kind, lv_event_cb_t cb, void* user) {
  lv_obj_t* b = lv_button_create(parent);
  applyKind(b, kind);
  lv_obj_set_size(b, w, h);
  lv_obj_t* l = lv_label_create(b);
  lv_obj_set_style_text_font(l, font, 0);
  lv_label_set_text(l, text);
  lv_obj_center(l);
  if (cb) lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, user);
  return b;
}

void ui_button_set_text(lv_obj_t* btn, const char* text) {
  lv_obj_t* l = lv_obj_get_child(btn, 0);
  if (l) { lv_label_set_text(l, text); lv_obj_center(l); }
}

void ui_button_set_kind(lv_obj_t* btn, BtnKind kind) { applyKind(btn, kind); }

// ---------------------------------------------------------------- toast
void ui_toast(const char* msg) {
  lv_obj_t* t = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(t);
  lv_obj_add_style(t, theme_style(ST_MODAL), 0);
  lv_obj_set_style_pad_hor(t, 18, 0);
  lv_obj_set_style_pad_ver(t, 10, 0);
  lv_obj_set_size(t, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_remove_flag(t, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  lv_obj_t* l = lv_label_create(t);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_16, 0);
  lv_label_set_text(l, msg);
  lv_obj_align(t, LV_ALIGN_BOTTOM_MID, 0, -22);
  lv_obj_delete_delayed(t, 2400);
}

// ---------------------------------------------------------------- dialogs
static lv_obj_t* s_modal = nullptr;
static void (*s_onOk)(void*) = nullptr;
static void* s_okUser = nullptr;

static void closeModal() { if (s_modal) { lv_obj_delete(s_modal); s_modal = nullptr; } }
bool ui_modal_active() { return s_modal != nullptr; }

static void okCb(lv_event_t* e) {
  (void)e;
  void (*cb)(void*) = s_onOk; void* u = s_okUser;
  closeModal();
  if (cb) cb(u);
}
static void cancelCb(lv_event_t* e) { (void)e; closeModal(); }

void ui_confirm(const char* title, const char* msg, const char* okText, const char* cancelText,
                BtnKind okKind, void (*onOk)(void*), void* user) {
  closeModal();
  s_onOk = onOk; s_okUser = user;
  s_modal = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(s_modal);
  lv_obj_set_size(s_modal, SB_SCREEN_W_UI, SB_SCREEN_H_UI);
  lv_obj_set_style_bg_color(s_modal, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(s_modal, 150, 0);
  lv_obj_add_flag(s_modal, LV_OBJ_FLAG_CLICKABLE);       // swallow touches behind
  lv_obj_remove_flag(s_modal, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* box = lv_obj_create(s_modal);
  lv_obj_remove_style_all(box);
  lv_obj_add_style(box, theme_style(ST_MODAL), 0);
  lv_obj_set_size(box, 420, 200);
  lv_obj_center(box);
  lv_obj_remove_flag(box, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* t = ui_label(box, &lv_font_montserrat_24, title);
  lv_obj_align(t, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_t* m = ui_label(box, &lv_font_montserrat_16, msg, true);
  lv_obj_set_width(m, 388);
  lv_label_set_long_mode(m, LV_LABEL_LONG_WRAP);
  lv_obj_align(m, LV_ALIGN_TOP_LEFT, 0, 44);

  if (cancelText) {
    lv_obj_t* cancel = ui_button(box, 180, 48, cancelText, &lv_font_montserrat_16, BTN_GLASS, cancelCb);
    lv_obj_align(cancel, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_t* ok = ui_button(box, 180, 48, okText, &lv_font_montserrat_16, okKind, okCb);
    lv_obj_align(ok, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  } else {
    lv_obj_t* ok = ui_button(box, 388, 48, okText, &lv_font_montserrat_16, okKind, okCb);
    lv_obj_align(ok, LV_ALIGN_BOTTOM_MID, 0, 0);
  }
}

// ---------------------------------------------------------------- navigation
static lv_obj_t* s_dash = nullptr;
static lv_obj_t* s_cur = nullptr;
static void (*s_curClose)() = nullptr;

struct OverlayMeta { void (*onClose)(); };

#if SB_UI_TRANSITIONS
static void fadeExec(void* o, int32_t v) { lv_obj_set_style_opa((lv_obj_t*)o, (lv_opa_t)v, 0); }
static void fade(lv_obj_t* o) {
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, o);
  lv_anim_set_exec_cb(&a, fadeExec);
  lv_anim_set_values(&a, 0, 255);
  lv_anim_set_duration(&a, 220);
  lv_anim_start(&a);
}
#else
// Fade animations redraw the whole 800x480 screen every frame (~200 ms each on this
// display path), which looks like a glitch. Screens now switch in a single redraw.
static void fade(lv_obj_t* o) { (void)o; }
#endif

void ui_nav_init(lv_obj_t* dashboard) { s_dash = dashboard; }

static void backCb(lv_event_t* e) { (void)e; ui_nav_close(); }

static OverlayMeta s_meta[8];
static int s_metaN = 0;

lv_obj_t* ui_overlay_create(lv_obj_t* parent, const char* title, void (*onClose)()) {
  lv_obj_t* o = lv_obj_create(parent);
  lv_obj_remove_style_all(o);
  lv_obj_set_size(o, SB_SCREEN_W_UI, SB_SCREEN_H_UI);
  lv_obj_add_style(o, theme_style(ST_OVERLAY), 0);
  lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
  if (s_metaN < 8) { s_meta[s_metaN].onClose = onClose; lv_obj_set_user_data(o, &s_meta[s_metaN]); s_metaN++; }

  lv_obj_t* back = ui_button(o, 56, 44, LV_SYMBOL_LEFT, &lv_font_montserrat_20, BTN_GLASS, backCb);
  lv_obj_set_pos(back, 12, 12);
  lv_obj_t* t = ui_label(o, &lv_font_montserrat_24, title);
  lv_obj_set_pos(t, 82, 20);
  return o;
}

void ui_nav_open(lv_obj_t* overlay) {
  if (s_cur) ui_nav_close();
  s_cur = overlay;
  OverlayMeta* m = (OverlayMeta*)lv_obj_get_user_data(overlay);
  s_curClose = m ? m->onClose : nullptr;
  if (s_dash) lv_obj_add_flag(s_dash, LV_OBJ_FLAG_HIDDEN);
  lv_obj_remove_flag(overlay, LV_OBJ_FLAG_HIDDEN);
  fade(overlay);
}

void ui_nav_close() {
  if (!s_cur) return;
  lv_obj_add_flag(s_cur, LV_OBJ_FLAG_HIDDEN);
  void (*cb)() = s_curClose;
  s_cur = nullptr; s_curClose = nullptr;
  if (cb) cb();
  if (s_dash) { lv_obj_remove_flag(s_dash, LV_OBJ_FLAG_HIDDEN); fade(s_dash); }
}

bool ui_nav_is_open(lv_obj_t* overlay) { return s_cur == overlay; }
