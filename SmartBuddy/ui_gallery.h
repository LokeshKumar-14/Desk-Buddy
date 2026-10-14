#pragma once
#include <lvgl.h>
lv_obj_t* ui_gallery_create(lv_obj_t* parent);
void      ui_gallery_open();
void      ui_gallery_tick();     // call after lv_timer_handler(): loads images, runs slideshow
