#include "ui_wifi.h"
#include "ui_common.h"
#include "wifi_manager.h"
#include "config.h"
#include <WiFi.h>

static lv_obj_t *s_ov, *s_status, *s_info, *s_dd, *s_ssid, *s_pass, *s_kb, *s_scanBtn;
static bool     s_open = false;
static bool     s_scanning = false;
static uint32_t s_key = 0;

static void kbHide() { lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN); }

static void taEvent(lv_event_t* e) {
  lv_obj_t* ta = (lv_obj_t*)lv_event_get_target(e);
  lv_event_code_t c = lv_event_get_code(e);
  if (c == LV_EVENT_FOCUSED || c == LV_EVENT_CLICKED) {
    lv_keyboard_set_textarea(s_kb, ta);
    lv_obj_remove_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
  } else if (c == LV_EVENT_READY || c == LV_EVENT_CANCEL) {
    kbHide();
  }
}
static void kbEvent(lv_event_t* e) {
  lv_event_code_t c = lv_event_get_code(e);
  if (c == LV_EVENT_READY || c == LV_EVENT_CANCEL) kbHide();
}

static void scanCb(lv_event_t* e) {
  (void)e;
  wifi_scan_start();
  s_scanning = true;
  ui_button_set_text(s_scanBtn, "Scanning...");
}
static void ddCb(lv_event_t* e) {
  (void)e;
  char buf[40];
  lv_dropdown_get_selected_str(s_dd, buf, sizeof(buf));
  if (buf[0] && strcmp(buf, "Scan results") != 0) lv_textarea_set_text(s_ssid, buf);
}
static void connectCb(lv_event_t* e) {
  (void)e;
  const char* ssid = lv_textarea_get_text(s_ssid);
  const char* pass = lv_textarea_get_text(s_pass);
  kbHide();
  if (!ssid[0]) { ui_toast("Enter a network name"); return; }
  const char* pw = pass;
  if (!pass[0] && strcmp(ssid, wifi_ssid()) == 0) {      // keep the saved password
    wifi_reconnect();
    ui_toast("Reconnecting...");
    return;
  }
  if (wifi_set_credentials(ssid, pw)) { ui_toast("Connecting..."); lv_textarea_set_text(s_pass, ""); }
  else ui_toast("Invalid name or password length");
}

static void styleInput(lv_obj_t* ta) {
  lv_obj_remove_style_all(ta);
  lv_obj_add_style(ta, theme_style(ST_INPUT), 0);
  lv_obj_set_style_pad_hor(ta, 10, 0);
  lv_obj_set_style_pad_ver(ta, 6, 0);
  lv_obj_set_style_text_font(ta, &lv_font_montserrat_16, 0);
  lv_obj_set_style_border_color(ta, lv_color_hex(theme().accent), LV_STATE_FOCUSED);
  lv_obj_set_style_border_width(ta, 2, LV_STATE_FOCUSED);
}

static void onClose() { s_open = false; if (s_kb) kbHide(); }

lv_obj_t* ui_wifi_create(lv_obj_t* parent) {
  s_ov = ui_overlay_create(parent, "Wi-Fi", onClose);

  lv_obj_t* st = ui_card(s_ov, 12, 68, 380, 190);
  s_status = ui_label(st, &lv_font_montserrat_24, "");
  lv_obj_align(s_status, LV_ALIGN_TOP_LEFT, 0, 0);
  s_info = ui_label(st, &lv_font_montserrat_16, "", true);
  lv_obj_set_width(s_info, 356);
  lv_obj_align(s_info, LV_ALIGN_TOP_LEFT, 0, 40);

  lv_obj_t* fm = ui_card(s_ov, 402, 68, 386, 190);
  lv_obj_set_style_pad_all(fm, 10, 0);
  s_dd = lv_dropdown_create(fm);
  lv_obj_remove_style_all(s_dd);
  lv_obj_add_style(s_dd, theme_style(ST_INPUT), 0);
  lv_obj_set_style_text_font(s_dd, &lv_font_montserrat_16, 0);
  lv_obj_set_style_pad_all(s_dd, 6, 0);
  lv_dropdown_set_options(s_dd, "Scan results");
  lv_obj_set_size(s_dd, 250, 36);
  lv_obj_set_pos(s_dd, 0, 0);
  lv_obj_add_event_cb(s_dd, ddCb, LV_EVENT_VALUE_CHANGED, nullptr);
  s_scanBtn = ui_button(fm, 108, 36, "Scan", &lv_font_montserrat_14, BTN_GLASS, scanCb);
  lv_obj_set_pos(s_scanBtn, 258, 0);

  s_ssid = lv_textarea_create(fm);
  styleInput(s_ssid);
  lv_textarea_set_one_line(s_ssid, true);
  lv_textarea_set_max_length(s_ssid, 32);
  lv_textarea_set_placeholder_text(s_ssid, "Network name (SSID)");
  lv_obj_set_size(s_ssid, 366, 36);
  lv_obj_set_pos(s_ssid, 0, 42);
  lv_obj_add_event_cb(s_ssid, taEvent, LV_EVENT_ALL, nullptr);

  s_pass = lv_textarea_create(fm);
  styleInput(s_pass);
  lv_textarea_set_one_line(s_pass, true);
  lv_textarea_set_password_mode(s_pass, true);
  lv_textarea_set_max_length(s_pass, 64);
  lv_textarea_set_placeholder_text(s_pass, "Password (blank = keep saved)");
  lv_obj_set_size(s_pass, 366, 36);
  lv_obj_set_pos(s_pass, 0, 84);
  lv_obj_add_event_cb(s_pass, taEvent, LV_EVENT_ALL, nullptr);

  lv_obj_t* cb = ui_button(fm, 366, 40, "Connect", &lv_font_montserrat_16, BTN_PRIMARY, connectCb);
  lv_obj_set_pos(cb, 0, 128);

  s_kb = lv_keyboard_create(s_ov);
  lv_obj_set_size(s_kb, SB_SCREEN_W, 214);
  lv_obj_align(s_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_add_style(s_kb, theme_style(ST_CARD), LV_PART_MAIN);
  lv_obj_set_style_pad_all(s_kb, 6, LV_PART_MAIN);
  lv_obj_add_style(s_kb, theme_style(ST_BTN), LV_PART_ITEMS);
  lv_obj_set_style_text_font(s_kb, &lv_font_montserrat_20, LV_PART_ITEMS);
  lv_obj_set_style_text_color(s_kb, lv_color_hex(theme().text), LV_PART_ITEMS);
  lv_obj_add_event_cb(s_kb, kbEvent, LV_EVENT_ALL, nullptr);
  lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
  return s_ov;
}

void ui_wifi_open() {
  lv_textarea_set_text(s_ssid, wifi_ssid());
  lv_textarea_set_text(s_pass, "");
  kbHide();
  s_key = 0;
  s_open = true;
  ui_nav_open(s_ov);
}

void ui_wifi_tick() {
  if (!s_open) return;
  if (s_scanning) {
    int r = wifi_scan_poll();
    if (r >= 0) {
      s_scanning = false;
      ui_button_set_text(s_scanBtn, "Scan");
      char opts[12 * 34 + 16];
      opts[0] = 0;
      for (int i = 0; i < r; i++) { if (i) strlcat(opts, "\n", sizeof(opts)); strlcat(opts, wifi_scan_ssid(i), sizeof(opts)); }
      if (r == 0) strlcpy(opts, "No networks found", sizeof(opts));
      lv_dropdown_set_options(s_dd, opts);
      if (r > 0) { lv_dropdown_set_selected(s_dd, 0); lv_textarea_set_text(s_ssid, wifi_scan_ssid(0)); }
    }
  }
  uint32_t key = wifi_version() * 13u + (uint32_t)wifi_state() + wifi_retry_in_sec() * 1000u;
  if (key == s_key) return;
  s_key = key;
  char a[48], b[200], ip[20];
  wifi_ip_str(ip, sizeof(ip));
  switch (wifi_state()) {
    case SBW_CONNECTED:
      strlcpy(a, "Connected", sizeof(a));
      snprintf(b, sizeof(b), "Network: %s\nIP address: %s\nSignal: %d dBm\nInternet: %s\nWeb page: http://%s/",
               wifi_ssid(), ip, wifi_rssi(), wifi_internet_ok() ? "available" : "not reachable", ip);
      break;
    case SBW_CONNECTING:
      strlcpy(a, "Connecting...", sizeof(a));
      snprintf(b, sizeof(b), "Network: %s\nPlease wait.", wifi_ssid());
      break;
    case SBW_FAILED:
      strlcpy(a, "Connection failed", sizeof(a));
      snprintf(b, sizeof(b), "Network: %s\nRetrying in %u s.\nCheck name/password or\nenter another network.",
               wifi_ssid(), (unsigned)wifi_retry_in_sec());
      break;
    default:
      strlcpy(a, "Not configured", sizeof(a));
      snprintf(b, sizeof(b), "Enter a network name and password.");
  }
  lv_label_set_text(s_status, a);
  lv_label_set_text(s_info, b);
}
