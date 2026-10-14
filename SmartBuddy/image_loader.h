// JPEG -> RGB565 decoder into PSRAM, plus a procedural default wallpaper.
#pragma once
#include <Arduino.h>
#include <lvgl.h>

struct SbImage {
  lv_image_dsc_t dsc;
  uint16_t*      pixels;      // PSRAM, RGB565
  uint16_t       w, h;
};

enum FitMode : uint8_t { FIT_COVER = 0, FIT_CONTAIN = 1 };

void        img_init(SbImage* im);
void        img_free(SbImage* im);
// Decodes path scaled to maxW x maxH (cover = crop to exactly that size,
// contain = keep aspect, never upscales). Baseline JPEG only.
bool        img_load_jpeg(const char* path, uint16_t maxW, uint16_t maxH, FitMode mode,
                          SbImage* out, const char** err);
bool        img_make_default_wallpaper(uint8_t themeIdx, SbImage* out);
bool        img_make_solid_wallpaper(uint32_t rgb, SbImage* out);
