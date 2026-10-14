// Daily checklist with subtasks. Items reset once per local day.
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"

struct DailySubtask {
  uint32_t id;
  char text[SB_DAILY_TEXT_MAX + 1];
  bool done;
};

struct DailyTask {
  uint32_t id;
  char text[SB_DAILY_TEXT_MAX + 1];
  bool done;
  uint8_t subCount;
  DailySubtask subs[SB_MAX_DAILY_SUBS];
};

void daily_begin();
void daily_loop();
int daily_count();
const DailyTask* daily_at(int i);
uint32_t daily_version();
bool daily_add(const char* text, const char* subtasksText);
bool daily_add_sub(uint32_t taskId, const char* text);
bool daily_toggle_task(uint32_t taskId);
bool daily_toggle_sub(uint32_t taskId, uint32_t subId);
bool daily_delete_task(uint32_t taskId);
bool daily_delete_sub(uint32_t taskId, uint32_t subId);
void daily_to_json(JsonArray arr);
