#pragma once
#include <lvgl.h>
void ui_dashboard_create();     // builds wallpaper + dashboard + all overlays on the active screen
void ui_dashboard_tick();       // cheap periodic refresh (call every loop)
