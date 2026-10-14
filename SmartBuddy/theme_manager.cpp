#include "theme_manager.h"

static const SbTheme kThemes[2] = {
  // Dark Glass
  { "Dark Glass",  0x05070F, 90,   0x121A2B, 140,  0xFFFFFF, 60,
    0xF4F7FB, 0xA9B4C6,   0xFFFFFF, 36,   0x4FA3FF, 0x4ADE80, 0xFF6B6B,  0x04101F,  0x000000, 110 },
  // Light Glass
  { "Light Glass", 0xFFFFFF, 40,   0xFFFFFF, 130,  0xFFFFFF, 190,
    0x142033, 0x4B5A70,   0xFFFFFF, 90,   0x1E6FE0, 0x1E9E55, 0xD64545,  0xFFFFFF,  0x1A2A44, 60 },
};

static lv_style_t s_st[ST_COUNT];
static bool       s_inited = false;
static uint8_t    s_idx = 0;

const SbTheme& theme() { return kThemes[s_idx]; }
uint8_t theme_index() { return s_idx; }
lv_style_t* theme_style(ThemeStyle id) { return &s_st[id]; }

static void build(const SbTheme& t) {
  for (int i = 0; i < ST_COUNT; i++) lv_style_reset(&s_st[i]);

  // --- glass card
  lv_style_t* s = &s_st[ST_CARD];
  lv_style_set_radius(s, SB_RADIUS_CARD);
  lv_style_set_bg_color(s, lv_color_hex(t.panel));
  lv_style_set_bg_opa(s, t.panelOpa);
  lv_style_set_border_color(s, lv_color_hex(t.border));
  lv_style_set_border_opa(s, t.borderOpa);
  lv_style_set_border_width(s, 1);
  lv_style_set_shadow_width(s, 24);
  lv_style_set_shadow_offset_y(s, 6);
  lv_style_set_shadow_color(s, lv_color_hex(t.shadow));
  lv_style_set_shadow_opa(s, t.shadowOpa);
  lv_style_set_text_color(s, lv_color_hex(t.text));
  lv_style_set_pad_all(s, 12);

  s = &s_st[ST_SCRIM];
  lv_style_set_bg_color(s, lv_color_hex(t.scrim));
  lv_style_set_bg_opa(s, t.scrimOpa);
  lv_style_set_border_width(s, 0);
  lv_style_set_radius(s, 0);

  s = &s_st[ST_OVERLAY];                     // full-screen sub page backdrop
  lv_style_set_bg_color(s, lv_color_hex(t.scrim));
  lv_style_set_bg_opa(s, (t.scrimOpa + 90 > 255) ? 255 : t.scrimOpa + 90);
  lv_style_set_border_width(s, 0);
  lv_style_set_radius(s, 0);
  lv_style_set_text_color(s, lv_color_hex(t.text));
  lv_style_set_pad_all(s, 0);

  s = &s_st[ST_MODAL];                       // dialogs / toasts (more opaque)
  lv_style_set_radius(s, SB_RADIUS_CARD);
  lv_style_set_bg_color(s, lv_color_hex(t.panel));
  lv_style_set_bg_opa(s, 235);
  lv_style_set_border_color(s, lv_color_hex(t.border));
  lv_style_set_border_opa(s, 120);
  lv_style_set_border_width(s, 1);
  lv_style_set_shadow_width(s, 30);
  lv_style_set_shadow_color(s, lv_color_hex(t.shadow));
  lv_style_set_shadow_opa(s, 150);
  lv_style_set_text_color(s, lv_color_hex(t.text));
  lv_style_set_pad_all(s, 16);

  // --- buttons
  s = &s_st[ST_BTN];
  lv_style_set_radius(s, SB_RADIUS_BTN);
  lv_style_set_bg_color(s, lv_color_hex(t.btn));
  lv_style_set_bg_opa(s, t.btnOpa);
  lv_style_set_border_color(s, lv_color_hex(t.border));
  lv_style_set_border_opa(s, t.borderOpa);
  lv_style_set_border_width(s, 1);
  lv_style_set_text_color(s, lv_color_hex(t.text));
  lv_style_set_shadow_width(s, 0);
  s = &s_st[ST_BTN_PR];
  lv_style_set_bg_color(s, lv_color_hex(t.accent));
  lv_style_set_bg_opa(s, 150);
  lv_style_set_border_opa(s, 220);
  lv_style_set_border_color(s, lv_color_hex(t.accent));
  lv_style_set_translate_y(s, 1);

  s = &s_st[ST_PRIMARY];
  lv_style_set_radius(s, SB_RADIUS_BTN);
  lv_style_set_bg_color(s, lv_color_hex(t.accent));
  lv_style_set_bg_opa(s, 235);
  lv_style_set_border_width(s, 0);
  lv_style_set_text_color(s, lv_color_hex(t.onAccent));
  s = &s_st[ST_PRIMARY_PR];
  lv_style_set_bg_opa(s, 150);
  lv_style_set_translate_y(s, 1);

  s = &s_st[ST_DANGER];
  lv_style_set_radius(s, SB_RADIUS_BTN);
  lv_style_set_bg_color(s, lv_color_hex(t.danger));
  lv_style_set_bg_opa(s, 215);
  lv_style_set_border_width(s, 0);
  lv_style_set_text_color(s, lv_color_hex(0xFFFFFF));
  s = &s_st[ST_DANGER_PR];
  lv_style_set_bg_opa(s, 140);
  lv_style_set_translate_y(s, 1);

  // --- text helpers
  lv_style_set_text_color(&s_st[ST_TEXT], lv_color_hex(t.text));
  lv_style_set_text_color(&s_st[ST_DIM], lv_color_hex(t.dim));
  s = &s_st[ST_DONE];
  lv_style_set_text_color(s, lv_color_hex(t.dim));
  lv_style_set_text_decor(s, LV_TEXT_DECOR_STRIKETHROUGH);
  lv_style_set_text_opa(s, 170);

  s = &s_st[ST_TODAY];
  lv_style_set_bg_color(s, lv_color_hex(t.accent));
  lv_style_set_bg_opa(s, 240);
  lv_style_set_radius(s, 8);
  lv_style_set_text_color(s, lv_color_hex(t.onAccent));

  s = &s_st[ST_BAR];
  lv_style_set_bg_color(s, lv_color_hex(t.text));
  lv_style_set_bg_opa(s, 40);
  lv_style_set_radius(s, 4);
  s = &s_st[ST_BAR_IND];
  lv_style_set_bg_color(s, lv_color_hex(t.accent));
  lv_style_set_bg_opa(s, 255);
  lv_style_set_radius(s, 4);

  s = &s_st[ST_INPUT];
  lv_style_set_radius(s, 10);
  lv_style_set_bg_color(s, lv_color_hex(t.scrim));
  lv_style_set_bg_opa(s, 110);
  lv_style_set_border_color(s, lv_color_hex(t.border));
  lv_style_set_border_opa(s, 150);
  lv_style_set_border_width(s, 1);
  lv_style_set_text_color(s, lv_color_hex(t.text));

  s = &s_st[ST_SCROLLBAR];
  lv_style_set_bg_color(s, lv_color_hex(t.text));
  lv_style_set_bg_opa(s, 90);
  lv_style_set_width(s, 4);
  lv_style_set_radius(s, 2);
}

void theme_init() {
  if (s_inited) return;
  for (int i = 0; i < ST_COUNT; i++) lv_style_init(&s_st[i]);
  s_inited = true;
  build(kThemes[s_idx]);
}

void theme_apply(uint8_t index) {
  s_idx = index ? 1 : 0;
  if (!s_inited) { theme_init(); return; }
  build(kThemes[s_idx]);
  lv_obj_report_style_change(NULL);
}
