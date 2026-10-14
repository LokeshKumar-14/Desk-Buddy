// Display + touch + LVGL bring-up (hardware init preserved from the Elecrow example).
#pragma once
#include <stdint.h>
bool display_begin();     // false if the RGB panel failed to start (check PSRAM = OPI)
void display_set_brightness(uint8_t percent);
