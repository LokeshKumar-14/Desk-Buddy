#pragma once
#include <lvgl.h>
lv_obj_t* ui_timer_create(lv_obj_t* parent, int x, int y, int w, int h);
void      ui_timer_tick();
