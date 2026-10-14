// Centralised glass theme: colours, opacities, radii, shared LVGL styles.
#pragma once
#include <lvgl.h>

enum ThemeStyle : uint8_t {
  ST_CARD, ST_SCRIM, ST_OVERLAY, ST_MODAL,
  ST_BTN, ST_BTN_PR, ST_PRIMARY, ST_PRIMARY_PR, ST_DANGER, ST_DANGER_PR,
  ST_TEXT, ST_DIM, ST_DONE, ST_TODAY, ST_BAR, ST_BAR_IND, ST_INPUT, ST_SCROLLBAR,
  ST_COUNT
};

#define SB_RADIUS_CARD   18
#define SB_RADIUS_BTN    14
#define SB_OPA_GLASS     LV_OPA_50

struct SbTheme {
  const char* name;
  uint32_t scrim;     uint8_t scrimOpa;     // wallpaper tint for legibility
  uint32_t panel;     uint8_t panelOpa;     // glass card fill
  uint32_t border;    uint8_t borderOpa;
  uint32_t text, dim;
  uint32_t btn;       uint8_t btnOpa;
  uint32_t accent, good, danger;
  uint32_t onAccent;                         // text on accent buttons
  uint32_t shadow;    uint8_t shadowOpa;
};

void              theme_init();                 // build styles (once)
void              theme_apply(uint8_t index);   // 0 dark, 1 light; refreshes all widgets
uint8_t           theme_index();
const SbTheme&    theme();
lv_style_t*       theme_style(ThemeStyle id);
