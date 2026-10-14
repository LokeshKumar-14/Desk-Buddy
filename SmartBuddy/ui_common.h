// Shared UI helpers: glass cards, buttons, labels, toast, confirm dialog, navigation.
#pragma once
#include <lvgl.h>
#include "theme_manager.h"

enum BtnKind : uint8_t { BTN_GLASS = 0, BTN_PRIMARY, BTN_DANGER };

lv_obj_t* ui_card(lv_obj_t* parent, int x, int y, int w, int h);
lv_obj_t* ui_label(lv_obj_t* parent, const lv_font_t* font, const char* text, bool dim = false);
// Button with a centred label (text or LV_SYMBOL_*). Pressed state is visible.
lv_obj_t* ui_button(lv_obj_t* parent, int w, int h, const char* text, const lv_font_t* font,
                    BtnKind kind, lv_event_cb_t cb, void* user = nullptr);
void      ui_button_set_text(lv_obj_t* btn, const char* text);
void      ui_button_set_kind(lv_obj_t* btn, BtnKind kind);   // restyle (e.g. play/pause)

void ui_toast(const char* msg);
// Modal dialogs on the top layer. onOk may be null. cancelText null => single button.
void ui_confirm(const char* title, const char* msg, const char* okText, const char* cancelText,
                BtnKind okKind, void (*onOk)(void*), void* user);

// Overlay navigation (full-screen sub pages above the dashboard).
void ui_nav_init(lv_obj_t* dashboard);
lv_obj_t* ui_overlay_create(lv_obj_t* parent, const char* title, void (*onClose)());
void ui_nav_open(lv_obj_t* overlay);
void ui_nav_close();
bool ui_nav_is_open(lv_obj_t* overlay);
bool ui_modal_active();
