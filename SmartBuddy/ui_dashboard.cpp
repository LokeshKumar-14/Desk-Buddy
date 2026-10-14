#include "ui_dashboard.h"
#include "ui_common.h"
#include "ui_calendar.h"
#include "ui_todo.h"
#include "ui_timer.h"
#include "ui_gallery.h"
#include "ui_settings.h"
#include "ui_wifi.h"
#include "ui_daily.h"
#include "theme_manager.h"
#include "time_manager.h"
#include "weather_manager.h"
#include "wifi_manager.h"
#include "storage_manager.h"
#include "wallpaper_manager.h"
#include "config.h"
#include "debug_redraw.h"

static lv_obj_t *s_wall, *s_dash;
static lv_obj_t *s_time, *s_sec, *s_date, *s_icoWifi, *s_icoSd;
static lv_obj_t *s_wxIcon, *s_wxTemp, *s_wxUnit, *s_wxDesc, *s_wxHum, *s_wxFoot;
static uint32_t s_wallVer = 0;
static int      s_iconKind = -1;
static uint32_t s_iconTheme = 99;

static void setText(lv_obj_t* l, const char* t) {
  if (strcmp(lv_label_get_text(l), t) != 0) lv_label_set_text(l, t);
}
// The macros tag every write with the widget's variable name for the redraw debugger.
#define SETTEXT(o, t)  do { SB_SCOPE(#o); setText(o, t); } while (0)

// ---------------------------------------------------------------- weather icons
static lv_obj_t* shape(lv_obj_t* p, int x, int y, int w, int h, int radius, uint32_t col, uint8_t opa = 255) {
  lv_obj_t* o = lv_obj_create(p);
  lv_obj_remove_style_all(o);
  lv_obj_set_pos(o, x, y);
  lv_obj_set_size(o, w, h);
  lv_obj_set_style_radius(o, radius, 0);
  lv_obj_set_style_bg_color(o, lv_color_hex(col), 0);
  lv_obj_set_style_bg_opa(o, opa, 0);
  lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  return o;
}
static void cloud(lv_obj_t* p, int ox, int oy, uint32_t col) {
  shape(p, ox + 4, oy + 16, 40, 18, 9, col);
  shape(p, ox + 6, oy + 6, 20, 20, LV_RADIUS_CIRCLE, col);
  shape(p, ox + 18, oy + 1, 26, 26, LV_RADIUS_CIRCLE, col);
}
static void sun(lv_obj_t* p, int cx, int cy, int body, int rayR, int rayD, uint32_t col) {
  shape(p, cx - body / 2, cy - body / 2, body, body, LV_RADIUS_CIRCLE, col);
  static const int8_t ux[8] = { 0, 7, 10, 7, 0, -7, -10, -7 };       // unit * 10
  static const int8_t uy[8] = { -10, -7, 0, 7, 10, 7, 0, -7 };
  for (int i = 0; i < 8; i++)
    shape(p, cx + ux[i] * rayR / 10 - rayD / 2, cy + uy[i] * rayR / 10 - rayD / 2, rayD, rayD, LV_RADIUS_CIRCLE, col);
}
static void drawIcon(WxIcon k) {
  lv_obj_clean(s_wxIcon);
  lv_obj_t* p = s_wxIcon;
  switch (k) {
    case WXI_SUN:   sun(p, 28, 28, 26, 22, 6, 0xFFC933); break;
    case WXI_MOON:  shape(p, 11, 11, 34, 34, LV_RADIUS_CIRCLE, 0xDDE3F5);
                    shape(p, 29, 17, 8, 8, LV_RADIUS_CIRCLE, 0xB9C3DE); shape(p, 18, 28, 10, 10, LV_RADIUS_CIRCLE, 0xB9C3DE); break;
    case WXI_PARTLY: sun(p, 18, 18, 16, 14, 4, 0xFFC933); cloud(p, 8, 20, 0xE6ECF7); break;
    case WXI_CLOUD: cloud(p, 4, 10, 0xDFE6F2); break;
    case WXI_FOG:   cloud(p, 4, 2, 0xC9D2E2); shape(p, 8, 38, 40, 4, 2, 0xC9D2E2); shape(p, 14, 46, 34, 4, 2, 0xC9D2E2); break;
    case WXI_RAIN:  cloud(p, 4, 2, 0xB8C4D6);
                    shape(p, 14, 38, 4, 12, 2, 0x5AB0FF); shape(p, 26, 40, 4, 12, 2, 0x5AB0FF); shape(p, 38, 38, 4, 12, 2, 0x5AB0FF); break;
    case WXI_SNOW:  cloud(p, 4, 2, 0xDDE5F0);
                    shape(p, 14, 42, 6, 6, LV_RADIUS_CIRCLE, 0xFFFFFF); shape(p, 26, 46, 6, 6, LV_RADIUS_CIRCLE, 0xFFFFFF); shape(p, 38, 42, 6, 6, LV_RADIUS_CIRCLE, 0xFFFFFF); break;
    case WXI_STORM: { cloud(p, 4, 0, 0x98A4BA);
                    lv_obj_t* b = lv_label_create(p);
                    lv_obj_set_style_text_font(b, &lv_font_montserrat_24, 0);
                    lv_obj_set_style_text_color(b, lv_color_hex(0xFFD23F), 0);
                    lv_label_set_text(b, LV_SYMBOL_CHARGE);
                    lv_obj_set_pos(b, 18, 30); } break;
  }
}

// ---------------------------------------------------------------- callbacks
static void picsCb(lv_event_t* e)  { (void)e; ui_gallery_open(); }
static void wifiCb(lv_event_t* e)  { (void)e; ui_wifi_open(); }
static void bgCb(lv_event_t* e)    { (void)e; wallpaper_request_next(); }
static void moreCb(lv_event_t* e)  { (void)e; ui_daily_open(); }
static void gearCb(lv_event_t* e)  { (void)e; ui_settings_open(); }
static void wxRefreshCb(lv_event_t* e) {
  (void)e;
  if (!wifi_connected()) { ui_toast("Wi-Fi is not connected"); return; }
  weather_request_refresh();
  ui_toast("Refreshing weather...");
}

// ---------------------------------------------------------------- build
void ui_dashboard_create() {
  theme_init();
  lv_obj_t* scr = lv_screen_active();
  lv_obj_remove_style_all(scr);
  lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  s_wall = lv_image_create(scr);
  lv_image_set_src(s_wall, wallpaper_dsc());
  lv_obj_set_pos(s_wall, 0, 0);
  s_wallVer = wallpaper_version();

  lv_obj_t* scrim = lv_obj_create(scr);
  lv_obj_remove_style_all(scrim);
  lv_obj_set_size(scrim, SB_SCREEN_W, SB_SCREEN_H);
  lv_obj_add_style(scrim, theme_style(ST_SCRIM), 0);
  lv_obj_remove_flag(scrim, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

  s_dash = lv_obj_create(scr);
  lv_obj_remove_style_all(s_dash);
  lv_obj_set_size(s_dash, SB_SCREEN_W, SB_SCREEN_H);
  lv_obj_add_style(s_dash, theme_style(ST_TEXT), 0);
  lv_obj_remove_flag(s_dash, LV_OBJ_FLAG_SCROLLABLE);
  ui_nav_init(s_dash);

  // ---- layout grid (800 x 480): margins 12, gaps 10
  const int LX = 12, LW = 380, RX = 402, RW = 386;

  // clock card
  lv_obj_t* clk = ui_card(s_dash, LX, 12, LW, 108);
  s_time = ui_label(clk, &lv_font_montserrat_48, "--:--");
  lv_obj_set_pos(s_time, 0, -8);
  s_sec = ui_label(clk, &lv_font_montserrat_20, "", true);
  lv_obj_set_pos(s_sec, 142, 14);
  s_date = ui_label(clk, &lv_font_montserrat_16, "Waiting for time sync", true);
  lv_obj_set_width(s_date, 300);
  lv_label_set_long_mode(s_date, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(s_date, 2, 52);
  lv_obj_t* gear = ui_button(clk, 44, 44, LV_SYMBOL_SETTINGS, &lv_font_montserrat_20, BTN_GLASS, gearCb);
  lv_obj_align(gear, LV_ALIGN_TOP_RIGHT, 0, -2);
  s_icoSd = ui_label(clk, &lv_font_montserrat_20, LV_SYMBOL_SD_CARD);
  lv_obj_align(s_icoSd, LV_ALIGN_TOP_RIGHT, -108, 8);
  s_icoWifi = ui_label(clk, &lv_font_montserrat_20, LV_SYMBOL_WIFI);
  lv_obj_align(s_icoWifi, LV_ALIGN_TOP_RIGHT, -64, 8);

  // weather card
  lv_obj_t* wx = ui_card(s_dash, LX, 130, LW, 92);
  s_wxIcon = lv_obj_create(wx);
  lv_obj_remove_style_all(s_wxIcon);
  lv_obj_set_size(s_wxIcon, 56, 56);
  lv_obj_set_pos(s_wxIcon, 0, 2);
  lv_obj_remove_flag(s_wxIcon, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  s_wxTemp = ui_label(wx, &lv_font_montserrat_40, "--");
  lv_obj_set_pos(s_wxTemp, 70, -8);
  s_wxUnit = ui_label(wx, &lv_font_montserrat_20, "", true);
  lv_obj_set_pos(s_wxUnit, 130, -2);
  s_wxDesc = ui_label(wx, &lv_font_montserrat_16, "No data");
  lv_obj_set_width(s_wxDesc, 150);
  lv_label_set_long_mode(s_wxDesc, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(s_wxDesc, 178, 0);
  s_wxHum = ui_label(wx, &lv_font_montserrat_14, "", true);
  lv_obj_set_pos(s_wxHum, 178, 24);
  s_wxFoot = ui_label(wx, &lv_font_montserrat_12, "", true);
  lv_obj_set_width(s_wxFoot, 290);
  lv_label_set_long_mode(s_wxFoot, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(s_wxFoot, 70, 52);
  lv_obj_t* rf = ui_button(wx, 36, 36, LV_SYMBOL_REFRESH, &lv_font_montserrat_16, BTN_GLASS, wxRefreshCb);
  lv_obj_align(rf, LV_ALIGN_TOP_RIGHT, 0, -2);

  // calendar
  ui_calendar_create(s_dash, LX, 232, LW, 174);

  // button row
  const int bw = (LW - 30) / 4;
  lv_obj_t* b1 = ui_button(s_dash, bw, 52, LV_SYMBOL_IMAGE "\nPICS", &lv_font_montserrat_14, BTN_GLASS, picsCb);
  lv_obj_set_pos(b1, LX, 416);
  lv_obj_t* b2 = ui_button(s_dash, bw, 52, LV_SYMBOL_WIFI "\nWIFI", &lv_font_montserrat_14, BTN_GLASS, wifiCb);
  lv_obj_set_pos(b2, LX + bw + 10, 416);
  lv_obj_t* b3 = ui_button(s_dash, bw, 52, LV_SYMBOL_TINT "\nBG", &lv_font_montserrat_14, BTN_GLASS, bgCb);
  lv_obj_set_pos(b3, LX + 2 * (bw + 10), 416);
  lv_obj_t* b4 = ui_button(s_dash, bw, 52, "...\nMORE", &lv_font_montserrat_14, BTN_GLASS, moreCb);
  lv_obj_set_pos(b4, LX + 3 * (bw + 10), 416);

  // right column
  ui_todo_create(s_dash, RX, 12, RW, 338);
  ui_timer_create(s_dash, RX, 360, RW, 108);

  // overlays (hidden until opened)
  ui_gallery_create(scr);
  ui_wifi_create(scr);
  ui_settings_create(scr);
  ui_daily_create(scr);
}

// ---------------------------------------------------------------- refresh
// Only touch an LVGL object when its value really changed - every style/position
// call invalidates the widget and forces a redraw (visible as constant refreshing).
static void setColorImpl(lv_obj_t* o, uint32_t col, uint32_t* cache) {
  if (*cache == col) return;
  *cache = col;
  lv_obj_set_style_text_color(o, lv_color_hex(col), 0);
}
static void setXImpl(lv_obj_t* o, int x, int* cache) {
  if (*cache == x) return;
  *cache = x;
  lv_obj_set_x(o, x);
}

#define SETCOLOR(o, c, cache) do { SB_SCOPE("color:" #o); setColorImpl(o, c, cache); } while (0)
#define SETX(o, x, cache)     do { SB_SCOPE("x:" #o); setXImpl(o, x, cache); } while (0)

static void updateClock() {
  struct tm lt;
  char t[16], sc[8], d[48];
  sc[0] = 0;
  if (time_local(&lt)) {
#if SB_CLOCK_24H
    strftime(t, sizeof(t), "%H:%M", &lt);
#else
    strftime(t, sizeof(t), "%I:%M", &lt);
#endif
#if SB_SHOW_SECONDS
    snprintf(sc, sizeof(sc), ":%02d", lt.tm_sec);
#endif
    strftime(d, sizeof(d), "%A, %d %B %Y", &lt);
#if !SB_CLOCK_24H
    strlcat(sc, lt.tm_hour >= 12 ? " PM" : " AM", sizeof(sc));
#endif
  } else {
    strlcpy(t, "--:--", sizeof(t));
    strlcpy(d, wifi_connected() ? "Syncing time..." : "Time not set - waiting for Wi-Fi", sizeof(d));
  }
  SETTEXT(s_time, t); SETTEXT(s_sec, sc); SETTEXT(s_date, d);
}

static void updateStatusIcons() {
  static uint32_t cWifi = 0xFFFFFFFF, cSd = 0xFFFFFFFF;
  uint32_t col;
  if (wifi_connected()) col = wifi_internet_ok() ? theme().good : 0xFFB020;
  else col = (wifi_state() == SBW_CONNECTING) ? 0xFFB020 : theme().danger;
  SETCOLOR(s_icoWifi, col, &cWifi);
  SETCOLOR(s_icoSd, storage_ok() ? theme().good : theme().danger, &cSd);
}

static void updateWeather() {
  static int xUnit = -1, xDesc = -1, xHum = -1;
  static uint32_t cFoot = 0xFFFFFFFF;
  WeatherData d; WeatherStatus st; bool stale;
  weather_get(&d, &st, &stale);
  char buf[96];
  if (!d.valid) {
    if (s_iconKind != 100 || s_iconTheme != 0) { SB_SCOPE("wxIcon:clean"); lv_obj_clean(s_wxIcon); s_iconKind = 100; s_iconTheme = 0; }
    SETTEXT(s_wxTemp, "--"); SETTEXT(s_wxUnit, "");
    const char* msg = "No weather yet";
    switch (st) {
      case WX_NO_LOCATION: msg = "Set a city on the phone page"; break;
      case WX_FETCHING:    msg = "Updating..."; break;
      case WX_ERROR:       msg = weather_error()[0] ? weather_error() : "Update failed"; break;
      default:             msg = wifi_connected() ? "Waiting for data..." : "Offline"; break;
    }
    SETTEXT(s_wxDesc, msg); SETTEXT(s_wxHum, ""); SETTEXT(s_wxFoot, "");
    return;
  }
  WxIcon ic = weather_icon(d.code, d.isDay);
  if (s_iconKind != (int)ic || s_iconTheme != 1) { SB_SCOPE("wxIcon:drawIcon"); drawIcon(ic); s_iconKind = (int)ic; s_iconTheme = 1; }

  snprintf(buf, sizeof(buf), "%.0f", d.tempC);
  if (strcmp(lv_label_get_text(s_wxTemp), buf) != 0) {
    { SB_SCOPE("s_wxTemp(text changed)"); lv_label_set_text(s_wxTemp, buf); }
    lv_obj_update_layout(s_wxTemp);                       // only when the temperature text changed
    int w = lv_obj_get_width(s_wxTemp);
    SETX(s_wxUnit, 70 + w + 4, &xUnit);
    SETX(s_wxDesc, 70 + w + 40, &xDesc);
    SETX(s_wxHum, 70 + w + 40, &xHum);
  }
  SETTEXT(s_wxUnit, "\xC2\xB0" "C");
  SETTEXT(s_wxDesc, weather_describe(d.code));
  if (d.humidity >= 0) snprintf(buf, sizeof(buf), "Humidity %d%%", d.humidity); else buf[0] = 0;
  SETTEXT(s_wxHum, buf);

  char upd[24] = "";
  if (d.updatedAt) {
    time_t t = (time_t)d.updatedAt; struct tm lt;
    localtime_r(&t, &lt);
    strftime(upd, sizeof(upd), "%H:%M", &lt);
  }
  const char* flag = (!wifi_internet_ok()) ? " | OFFLINE" : (stale ? " | STALE" : "");
  snprintf(buf, sizeof(buf), "%s | updated %s%s", d.place, upd[0] ? upd : "--", flag);
  SETTEXT(s_wxFoot, buf);
  SETCOLOR(s_wxFoot, (!wifi_internet_ok() || stale) ? 0xFFB020 : theme().dim, &cFoot);
}

void ui_dashboard_tick() {
  SB_SCOPE("ui_dashboard_tick");
  static uint32_t lastSlow = 0;
  static uint32_t lastHourlyToast = 0;
  uint32_t now = millis();

  if (wallpaper_version() != s_wallVer) {
    SB_SCOPE("wallpaper_changed");
    s_wallVer = wallpaper_version();
    lv_image_set_src(s_wall, wallpaper_dsc());
    lv_obj_invalidate(s_wall);
  }
  if (const char* m = wallpaper_take_message()) { SB_SCOPE("wallpaper_toast"); ui_toast(m); }

  { SB_SCOPE("ui_todo_tick");     ui_todo_tick(); }
  { SB_SCOPE("ui_timer_tick");    ui_timer_tick(); }
  { SB_SCOPE("ui_calendar_tick"); ui_calendar_tick(); }
  { SB_SCOPE("ui_gallery_tick");  ui_gallery_tick(); }
  { SB_SCOPE("ui_wifi_tick");     ui_wifi_tick(); }
  { SB_SCOPE("ui_settings_tick"); ui_settings_tick(); }
  { SB_SCOPE("ui_daily_tick");    ui_daily_tick(); }

  if (now - lastSlow >= 1000 || lastSlow == 0) {
    lastSlow = now ? now : 1;
    { SB_SCOPE("updateClock");       updateClock(); }
    { SB_SCOPE("updateStatusIcons"); updateStatusIcons(); }
    { SB_SCOPE("updateWeather");     updateWeather(); }
  }
  if (now - lastHourlyToast >= 3600000UL) {
    lastHourlyToast = now;
    ui_toast("Hourly reminder: check your daily tasks");
  } else if (lastHourlyToast == 0) {
    lastHourlyToast = now;
  }
}
