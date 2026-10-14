#include "daily_manager.h"
#include "storage_manager.h"
#include "time_manager.h"

static DailyTask s_d[SB_MAX_DAILY_TASKS];
static int s_n = 0;
static uint32_t s_nextTask = 1;
static uint32_t s_nextSub = 1;
static uint32_t s_ver = 1;
static bool s_dirty = false;
static uint32_t s_dirtyAt = 0;
static int s_lastDay = 0;
static uint32_t s_lastCheck = 0;

static int todayKey() {
  struct tm lt;
  if (!time_local(&lt)) return 0;
  return (lt.tm_year + 1900) * 10000 + (lt.tm_mon + 1) * 100 + lt.tm_mday;
}

static void changed() { s_ver++; s_dirty = true; s_dirtyAt = millis(); }

static bool cleanText(const char* in, char* out) {
  if (!in) return false;
  while (*in && isspace((unsigned char)*in)) in++;
  size_t n = 0;
  while (*in && n < SB_DAILY_TEXT_MAX) {
    unsigned char c = (unsigned char)*in++;
    out[n++] = (c < 32 || c == 127) ? ' ' : (char)c;
  }
  while (n > 0 && isspace((unsigned char)out[n - 1])) n--;
  out[n] = 0;
  return n > 0;
}

static void recompute(DailyTask& t) {
  if (t.subCount == 0) return;
  for (uint8_t i = 0; i < t.subCount; i++) {
    if (!t.subs[i].done) { t.done = false; return; }
  }
  t.done = true;
}

static void resetAllForNewDay(int day) {
  if (!day || day == s_lastDay) return;
  for (int i = 0; i < s_n; i++) {
    s_d[i].done = false;
    for (uint8_t j = 0; j < s_d[i].subCount; j++) s_d[i].subs[j].done = false;
  }
  s_lastDay = day;
  changed();
}

static bool saveNow() {
  JsonDocument doc;
  doc["nextTask"] = s_nextTask;
  doc["nextSub"] = s_nextSub;
  doc["lastDay"] = s_lastDay;
  JsonArray arr = doc["daily"].to<JsonArray>();
  for (int i = 0; i < s_n; i++) {
    JsonObject o = arr.add<JsonObject>();
    o["id"] = s_d[i].id;
    o["t"] = s_d[i].text;
    o["d"] = s_d[i].done ? 1 : 0;
    JsonArray subs = o["subs"].to<JsonArray>();
    for (uint8_t j = 0; j < s_d[i].subCount; j++) {
      JsonObject so = subs.add<JsonObject>();
      so["id"] = s_d[i].subs[j].id;
      so["t"] = s_d[i].subs[j].text;
      so["d"] = s_d[i].subs[j].done ? 1 : 0;
    }
  }
  return storage_save_json(SB_FILE_DAILY, doc);
}

void daily_begin() {
  s_n = 0; s_nextTask = 1; s_nextSub = 1; s_lastDay = todayKey();
  JsonDocument doc;
  if (storage_load_json(SB_FILE_DAILY, doc)) {
    s_nextTask = doc["nextTask"] | 1u;
    s_nextSub = doc["nextSub"] | 1u;
    s_lastDay = doc["lastDay"] | s_lastDay;
    JsonArrayConst arr = doc["daily"].as<JsonArrayConst>();
    for (JsonObjectConst o : arr) {
      if (s_n >= SB_MAX_DAILY_TASKS) break;
      DailyTask t;
      memset(&t, 0, sizeof(t));
      t.id = o["id"] | 0u;
      if (!t.id || !cleanText(o["t"] | "", t.text)) continue;
      t.done = (o["d"] | 0) != 0;
      JsonArrayConst subs = o["subs"].as<JsonArrayConst>();
      for (JsonObjectConst so : subs) {
        if (t.subCount >= SB_MAX_DAILY_SUBS) break;
        DailySubtask& st = t.subs[t.subCount];
        st.id = so["id"] | 0u;
        if (!st.id || !cleanText(so["t"] | "", st.text)) continue;
        st.done = (so["d"] | 0) != 0;
        t.subCount++;
        if (st.id >= s_nextSub) s_nextSub = st.id + 1;
      }
      recompute(t);
      s_d[s_n++] = t;
      if (t.id >= s_nextTask) s_nextTask = t.id + 1;
    }
  }
  resetAllForNewDay(todayKey());
  s_dirty = false;
  s_ver++;
}

void daily_loop() {
  uint32_t now = millis();
  if (now - s_lastCheck >= 60000UL || s_lastCheck == 0) {
    s_lastCheck = now ? now : 1;
    resetAllForNewDay(todayKey());
  }
  if (s_dirty && now - s_dirtyAt >= 800) {
    if (saveNow()) s_dirty = false; else s_dirtyAt = now + 8000;
  }
}

int daily_count() { return s_n; }
const DailyTask* daily_at(int i) { return (i >= 0 && i < s_n) ? &s_d[i] : nullptr; }
uint32_t daily_version() { return s_ver; }

bool daily_add(const char* text, const char* subtasksText) {
  if (s_n >= SB_MAX_DAILY_TASKS) return false;
  DailyTask& t = s_d[s_n];
  memset(&t, 0, sizeof(t));
  if (!cleanText(text, t.text)) return false;
  t.id = s_nextTask++;
  if (subtasksText) {
    const char* p = subtasksText;
    while (*p && t.subCount < SB_MAX_DAILY_SUBS) {
      char line[SB_DAILY_TEXT_MAX + 1];
      size_t n = 0;
      while (*p && *p != '\n' && *p != ',' && n < SB_DAILY_TEXT_MAX) line[n++] = *p++;
      while (*p == '\n' || *p == ',') p++;
      line[n] = 0;
      DailySubtask& st = t.subs[t.subCount];
      if (cleanText(line, st.text)) { st.id = s_nextSub++; st.done = false; t.subCount++; }
    }
  }
  s_n++;
  changed();
  return true;
}

bool daily_add_sub(uint32_t taskId, const char* text) {
  for (int i = 0; i < s_n; i++) {
    if (s_d[i].id != taskId || s_d[i].subCount >= SB_MAX_DAILY_SUBS) continue;
    DailySubtask& st = s_d[i].subs[s_d[i].subCount];
    memset(&st, 0, sizeof(st));
    if (!cleanText(text, st.text)) return false;
    st.id = s_nextSub++;
    s_d[i].subCount++;
    recompute(s_d[i]);
    changed();
    return true;
  }
  return false;
}

bool daily_toggle_task(uint32_t taskId) {
  for (int i = 0; i < s_n; i++) {
    if (s_d[i].id != taskId) continue;
    bool nd = !s_d[i].done;
    s_d[i].done = nd;
    for (uint8_t j = 0; j < s_d[i].subCount; j++) s_d[i].subs[j].done = nd;
    changed();
    return true;
  }
  return false;
}

bool daily_toggle_sub(uint32_t taskId, uint32_t subId) {
  for (int i = 0; i < s_n; i++) {
    if (s_d[i].id != taskId) continue;
    for (uint8_t j = 0; j < s_d[i].subCount; j++) {
      if (s_d[i].subs[j].id != subId) continue;
      s_d[i].subs[j].done = !s_d[i].subs[j].done;
      recompute(s_d[i]);
      changed();
      return true;
    }
  }
  return false;
}

bool daily_delete_task(uint32_t taskId) {
  for (int i = 0; i < s_n; i++) {
    if (s_d[i].id != taskId) continue;
    for (int j = i; j < s_n - 1; j++) s_d[j] = s_d[j + 1];
    s_n--;
    changed();
    return true;
  }
  return false;
}

bool daily_delete_sub(uint32_t taskId, uint32_t subId) {
  for (int i = 0; i < s_n; i++) {
    if (s_d[i].id != taskId) continue;
    for (uint8_t j = 0; j < s_d[i].subCount; j++) {
      if (s_d[i].subs[j].id != subId) continue;
      for (uint8_t k = j; k + 1 < s_d[i].subCount; k++) s_d[i].subs[k] = s_d[i].subs[k + 1];
      s_d[i].subCount--;
      recompute(s_d[i]);
      changed();
      return true;
    }
  }
  return false;
}

void daily_to_json(JsonArray arr) {
  for (int i = 0; i < s_n; i++) {
    JsonObject o = arr.add<JsonObject>();
    o["id"] = s_d[i].id;
    o["text"] = s_d[i].text;
    o["done"] = s_d[i].done;
    JsonArray subs = o["subs"].to<JsonArray>();
    for (uint8_t j = 0; j < s_d[i].subCount; j++) {
      JsonObject so = subs.add<JsonObject>();
      so["id"] = s_d[i].subs[j].id;
      so["text"] = s_d[i].subs[j].text;
      so["done"] = s_d[i].subs[j].done;
    }
  }
}
