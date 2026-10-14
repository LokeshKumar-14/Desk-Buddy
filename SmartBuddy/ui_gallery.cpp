#include "ui_gallery.h"
#include "ui_common.h"
#include "media_manager.h"
#include "image_loader.h"
#include "settings_manager.h"
#include "wifi_manager.h"
#include "config.h"

#define IMG_AREA_Y   60
#define IMG_AREA_H   (SB_SCREEN_H - 60 - 74)

static lv_obj_t *s_ov, *s_img, *s_msg, *s_counter, *s_name, *s_play;
static lv_obj_t *s_btnPrev, *s_btnNext, *s_btnDel;
static SbImage   s_cur;
static int       s_idx = 0;
static bool      s_playing = true;
static bool      s_needLoad = false;
static uint32_t  s_lastAdv = 0;
static uint32_t  s_listVer = 0;
static bool      s_open = false;

static void releaseImage() {
  if (s_img) lv_obj_add_flag(s_img, LV_OBJ_FLAG_HIDDEN);
  img_free(&s_cur);
}

static void onClose() {
  s_open = false; s_needLoad = false;
  if (s_img) lv_image_set_src(s_img, NULL);
  releaseImage();                       // free PSRAM when leaving the gallery
}

static void go(int delta) {
  int n = media_count(MEDIA_IMAGES);
  if (n <= 0) return;
  s_idx = ((s_idx + delta) % n + n) % n;
  s_needLoad = true;
  s_lastAdv = millis();
}
static void prevCb(lv_event_t* e) { (void)e; go(-1); }
static void nextCb(lv_event_t* e) { (void)e; go(+1); }
static void playCb(lv_event_t* e) {
  (void)e;
  s_playing = !s_playing;
  ui_button_set_text(s_play, s_playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
  s_lastAdv = millis();
}

static void doDelete(void*) {
  const char* nm = media_name(MEDIA_IMAGES, s_idx);
  char name[SB_MEDIA_NAME_MAX];
  strlcpy(name, nm, sizeof(name));
  if (media_delete(MEDIA_IMAGES, name)) ui_toast("Photo deleted");
  else ui_toast("Delete failed");
  int n = media_count(MEDIA_IMAGES);
  if (s_idx >= n) s_idx = n - 1;
  if (s_idx < 0) s_idx = 0;
  s_listVer = media_version(MEDIA_IMAGES);
  s_needLoad = true;
}
static void delCb(lv_event_t* e) {
  (void)e;
  if (media_count(MEDIA_IMAGES) == 0) return;
  ui_confirm("Delete photo?", media_name(MEDIA_IMAGES, s_idx), "Delete", "Cancel", BTN_DANGER, doDelete, nullptr);
}

lv_obj_t* ui_gallery_create(lv_obj_t* parent) {
  img_init(&s_cur);
  s_ov = ui_overlay_create(parent, "Gallery", onClose);
  s_counter = ui_label(s_ov, &lv_font_montserrat_20, "0 / 0");
  lv_obj_align(s_counter, LV_ALIGN_TOP_RIGHT, -16, 20);
  s_name = ui_label(s_ov, &lv_font_montserrat_14, "", true);
  lv_obj_set_width(s_name, 340);
  lv_label_set_long_mode(s_name, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_align(s_name, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(s_name, LV_ALIGN_TOP_MID, 0, 26);

  s_img = lv_image_create(s_ov);
  lv_obj_add_flag(s_img, LV_OBJ_FLAG_HIDDEN);

  s_msg = ui_label(s_ov, &lv_font_montserrat_20, "", true);
  lv_obj_set_width(s_msg, 600);
  lv_label_set_long_mode(s_msg, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(s_msg, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(s_msg, LV_ALIGN_CENTER, 0, -10);

  const int by = SB_SCREEN_H - 12 - 52;
  s_btnPrev = ui_button(s_ov, 130, 52, LV_SYMBOL_PREV, &lv_font_montserrat_24, BTN_GLASS, prevCb);
  lv_obj_set_pos(s_btnPrev, 150, by);
  s_play = ui_button(s_ov, 130, 52, LV_SYMBOL_PAUSE, &lv_font_montserrat_24, BTN_PRIMARY, playCb);
  lv_obj_set_pos(s_play, 295, by);
  s_btnNext = ui_button(s_ov, 130, 52, LV_SYMBOL_NEXT, &lv_font_montserrat_24, BTN_GLASS, nextCb);
  lv_obj_set_pos(s_btnNext, 440, by);
  s_btnDel = ui_button(s_ov, 130, 52, LV_SYMBOL_TRASH, &lv_font_montserrat_24, BTN_DANGER, delCb);
  lv_obj_set_pos(s_btnDel, 585, by);
  return s_ov;
}

void ui_gallery_open() {
  media_refresh(MEDIA_IMAGES);
  s_listVer = media_version(MEDIA_IMAGES);
  int n = media_count(MEDIA_IMAGES);
  if (s_idx >= n) s_idx = 0;
  s_playing = true;
  ui_button_set_text(s_play, LV_SYMBOL_PAUSE);
  s_needLoad = true;
  s_lastAdv = millis();
  s_open = true;
  ui_nav_open(s_ov);
}

static void showMessage(const char* m) {
  lv_obj_add_flag(s_img, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(s_msg, m);
  lv_obj_remove_flag(s_msg, LV_OBJ_FLAG_HIDDEN);
}

static void loadCurrent() {
  int n = media_count(MEDIA_IMAGES);
  char buf[96];
  if (n == 0) {
    char ip[20]; wifi_ip_str(ip, sizeof(ip));
    if (ip[0]) snprintf(buf, sizeof(buf), "No photos yet.\nUpload from your phone: http://%s/", ip);
    else snprintf(buf, sizeof(buf), "No photos yet.\nPut JPEG files in /SmartBuddy/images on the SD card.");
    lv_label_set_text(s_counter, "0 / 0");
    lv_label_set_text(s_name, "");
    showMessage(buf);
    releaseImage();
    return;
  }
  // try the requested photo; on failure skip forward (bounded)
  for (int attempt = 0; attempt < n; attempt++) {
    const char* nm = media_name(MEDIA_IMAGES, s_idx);
    snprintf(buf, sizeof(buf), "%d / %d", s_idx + 1, n);
    lv_label_set_text(s_counter, buf);
    lv_label_set_text(s_name, nm);
    char path[128];
    SbImage fresh;
    const char* err = nullptr;
    if (media_path(MEDIA_IMAGES, nm, path, sizeof(path)) &&
        img_load_jpeg(path, SB_SCREEN_W, IMG_AREA_H, FIT_CONTAIN, &fresh, &err)) {
            SbImage old = s_cur;
            s_cur = fresh;                                   // pehle copy karo
            lv_image_set_src(s_img, &s_cur.dsc);             // phir s_cur ka address do (fresh ka nahi)
            lv_obj_set_pos(s_img, (SB_SCREEN_W - s_cur.w) / 2, IMG_AREA_Y + (IMG_AREA_H - s_cur.h) / 2);
            lv_obj_remove_flag(s_img, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(s_msg, LV_OBJ_FLAG_HIDDEN);
            lv_obj_invalidate(s_img);
            img_free(&old);
            return;
    }
    Serial.printf("[GAL] %s: %s\n", nm, err ? err : "failed");
    if (n == 1 || attempt == n - 1) {
      snprintf(buf, sizeof(buf), "Cannot display this photo:\n%s", err ? err : "unsupported file");
      showMessage(buf);
      lv_image_set_src(s_img, NULL);
      releaseImage();
      return;
    }
    s_idx = (s_idx + 1) % n;          // skip the bad file
  }
}

void ui_gallery_tick() {
  if (!s_open) return;
  bool empty = media_count(MEDIA_IMAGES) == 0;
  if (empty) { lv_obj_add_state(s_btnDel, LV_STATE_DISABLED); }
  else lv_obj_remove_state(s_btnDel, LV_STATE_DISABLED);

  if (s_needLoad) { s_needLoad = false; loadCurrent(); s_lastAdv = millis(); return; }
  if (!empty && s_playing && !ui_modal_active() &&
      millis() - s_lastAdv >= (uint32_t)settings_get().slideshowSec * 1000UL && media_count(MEDIA_IMAGES) > 1) {
    go(+1);
  }
}
