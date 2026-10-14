// To-Do data model + persistence (/SmartBuddy/data/tasks.json).
#pragma once
#include <Arduino.h>
#include "config.h"

struct Task {
  uint32_t id;
  char     text[SB_TASK_TEXT_MAX + 1];
  bool     done;
  uint32_t created;     // epoch seconds, 0 = clock was not synced
  uint32_t completed;   // epoch seconds, 0 = not done / completed while clock unsynced
};

void        tasks_begin();
void        tasks_loop();                       // expiry + debounced save
int         tasks_count();                      // active first, then completed (newest first)
const Task* tasks_at(int i);
int         tasks_active_count();
int         tasks_done_count();
uint32_t    tasks_version();                    // bumps on every change
bool        tasks_add(const char* text);        // false: empty, full
bool        tasks_toggle(uint32_t id);
bool        tasks_delete(uint32_t id);
bool        tasks_save_ok();
