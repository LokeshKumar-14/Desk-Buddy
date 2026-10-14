#include "wallpaper_manager.h"
#include "media_manager.h"
#include "image_loader.h"
#include "settings_manager.h"
#include "theme_manager.h"
#include "config.h"

static SbImage  s_img;
static uint32_t s_ver = 0;
static char     s_cur[SB_MEDIA_NAME_MAX] = "";
static bool     s_pendNext = false;
static bool     s_pendColor = false;
static bool     s_colorMode = false;
static char     s_pendSel[SB_MEDIA_NAME_MAX] = "";
static char     s_msg[80] = "";

static void setMsg(const char* a, const char* b = "") { snprintf(s_msg, sizeof(s_msg), "%s%s", a, b); }

static bool loadDefault() {
  SbImage n;
  if (!img_make_solid_wallpaper(settings_get().bgColor, &n)) return false;
  SbImage old = s_img;
  s_img = n; s_cur[0] = 0; s_colorMode = true; s_ver++;
  img_free(&old);
  return true;
}

static bool loadFile(const char* name) {
  char path[128];
  if (!media_path(MEDIA_BACKGROUNDS, name, path, sizeof(path))) return false;
  SbImage n;
  const char* err = nullptr;
  if (!img_load_jpeg(path, SB_SCREEN_W, SB_SCREEN_H, FIT_COVER, &n, &err)) {
    Serial.printf("[BG] %s: %s\n", name, err ? err : "failed");
    setMsg("Cannot load ", name);
    return false;
  }
  SbImage old = s_img;
  s_img = n; strlcpy(s_cur, name, sizeof(s_cur)); s_colorMode = false; s_ver++;
  img_free(&old);
  if (strcmp(settings_get().bg, name)) strlcpy(settings_edit().bg, name, SB_MEDIA_NAME_MAX);
  if (settings_get().bgColorOnly) settings_edit().bgColorOnly = false;
  return true;
}

static void fallbackDefault() {
  loadDefault();
  if (settings_get().bg[0]) settings_edit().bg[0] = 0;
  if (!settings_get().bgColorOnly) settings_edit().bgColorOnly = true;
}

void wallpaper_begin() {
  img_init(&s_img);
  media_refresh(MEDIA_BACKGROUNDS);
  int n = media_count(MEDIA_BACKGROUNDS);
  if (settings_get().bgColorOnly || n == 0) { fallbackDefault(); return; }
  int start = media_find(MEDIA_BACKGROUNDS, settings_get().bg);
  if (start < 0) start = 0;
  for (int i = 0; i < n; i++) {
    if (loadFile(media_name(MEDIA_BACKGROUNDS, (start + i) % n))) return;
  }
  fallbackDefault();
}

void wallpaper_request_next() { s_pendNext = true; }
void wallpaper_request_color() { s_pendColor = true; }

bool wallpaper_request_select(const char* name) {
  if (media_find(MEDIA_BACKGROUNDS, name) < 0) return false;
  strlcpy(s_pendSel, name, sizeof(s_pendSel));
  return true;
}

void wallpaper_on_deleted(const char* name) {
  if (strcmp(name, s_cur) == 0) s_pendNext = true;
}

void wallpaper_on_theme_changed() { if (s_colorMode) loadDefault(); }

void wallpaper_loop() {
  if (s_pendColor) {
    s_pendColor = false;
    fallbackDefault();
    setMsg("Background color only");
    return;
  }
  if (s_pendSel[0]) {
    char nm[SB_MEDIA_NAME_MAX];
    strlcpy(nm, s_pendSel, sizeof(nm)); s_pendSel[0] = 0;
    media_refresh(MEDIA_BACKGROUNDS);
    if (loadFile(nm)) setMsg("Wallpaper: ", nm);
    return;
  }
  if (!s_pendNext) return;
  s_pendNext = false;
  media_refresh(MEDIA_BACKGROUNDS);
  int n = media_count(MEDIA_BACKGROUNDS);
  if (n == 0) { fallbackDefault(); setMsg("Default wallpaper"); return; }
  int cur = media_find(MEDIA_BACKGROUNDS, s_cur);
  if (s_colorMode) {
    if (loadFile(media_name(MEDIA_BACKGROUNDS, 0))) { setMsg("Wallpaper: ", s_cur); return; }
  } else if (cur >= 0 && cur + 1 >= n) {
    fallbackDefault();
    setMsg("Background color only");
    return;
  } else {
    int start = cur < 0 ? 0 : cur + 1;
    for (int idx = start; idx < n; idx++) {
      const char* nm = media_name(MEDIA_BACKGROUNDS, idx);
      if (loadFile(nm)) { setMsg("Wallpaper: ", nm); return; }
    }
  }
  fallbackDefault();
  setMsg("Background color only");
}

const lv_image_dsc_t* wallpaper_dsc() { return &s_img.dsc; }
uint32_t wallpaper_version() { return s_ver; }
const char* wallpaper_current() { return s_cur; }
bool wallpaper_color_mode() { return s_colorMode; }
const char* wallpaper_take_message() {
  static char out[80];
  if (!s_msg[0]) return nullptr;
  strlcpy(out, s_msg, sizeof(out)); s_msg[0] = 0;
  return out;
}
