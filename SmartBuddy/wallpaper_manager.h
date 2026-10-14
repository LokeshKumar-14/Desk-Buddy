// Wallpaper selection, loading and persistence.
#pragma once
#include <lvgl.h>

void                 wallpaper_begin();
void                 wallpaper_loop();                 // performs pending loads (after LVGL rendered)
void                 wallpaper_request_next();         // BG button
bool                 wallpaper_request_select(const char* name);   // from web, must be listed
void                 wallpaper_request_color();                    // no image, only chosen color
void                 wallpaper_on_deleted(const char* name);
void                 wallpaper_on_theme_changed();     // regenerates the built-in default
const lv_image_dsc_t* wallpaper_dsc();
uint32_t             wallpaper_version();              // bumps when the image changed
const char*          wallpaper_current();              // "" = built-in default
bool                 wallpaper_color_mode();
const char*          wallpaper_take_message();         // one-shot status text for a toast
