#include "task_manager.h"
#include "storage_manager.h"
#include "time_manager.h"

static Task     s_t[SB_MAX_TASKS];
static int      s_n = 0;
static uint32_t s_next = 1;
static uint32_t s_ver = 1;
static bool     s_dirty = false;
static uint32_t s_dirtyAt = 0;
static bool     s_saveOk = true;
static uint32_t s_lastExpiry = 0;
static bool     s_stamped = false;

static bool before(const Task& a, const Task& b) {
  if (a.done != b.done) return !a.done;
  if (!a.done) return a.id < b.id;
  uint32_t ca = a.completed ? a.completed : 0xFFFFFFFFu;
  uint32_t cb = b.completed ? b.completed : 0xFFFFFFFFu;
  if (ca != cb) return ca > cb;
  return a.id > b.id;
}

static void sortTasks() {
  for (int i = 1; i < s_n; i++) {
    Task k = s_t[i];
    int j = i - 1;
    while (j >= 0 && before(k, s_t[j])) { s_t[j + 1] = s_t[j]; j--; }
    s_t[j + 1] = k;
  }
}

static void changed() { s_ver++; s_dirty = true; s_dirtyAt = millis(); }

static bool cleanText(const char* in, char* out) {
  if (!in) return false;
  while (*in && isspace((unsigned char)*in)) in++;
  size_t n = 0;
  while (*in && n < SB_TASK_TEXT_MAX) {
    unsigned char c = (unsigned char)*in++;
    out[n++] = (c < 32 || c == 127) ? ' ' : (char)c;
  }
  // do not cut a UTF-8 sequence in half
  if (*in && n > 0 && ((unsigned char)out[n - 1] & 0x80)) {
    while (n > 0 && ((unsigned char)out[n - 1] & 0xC0) == 0x80) n--;
    if (n > 0) n--;
  }
  while (n > 0 && isspace((unsigned char)out[n - 1])) n--;
  out[n] = 0;
  return n > 0;
}

void tasks_begin() {
  s_n = 0; s_next = 1;
  JsonDocument doc;
  if (!storage_load_json(SB_FILE_TASKS, doc)) { s_ver++; return; }
  JsonArrayConst arr = doc["tasks"].as<JsonArrayConst>();
  uint32_t maxId = 0;
  for (JsonObjectConst o : arr) {
    if (s_n >= SB_MAX_TASKS) break;
    Task t;
    memset(&t, 0, sizeof(t));
    t.id = o["id"] | 0u;
    if (t.id == 0) continue;
    bool dup = false;
    for (int i = 0; i < s_n; i++) if (s_t[i].id == t.id) dup = true;
    if (dup) continue;
    if (!cleanText(o["t"] | "", t.text)) continue;
    t.done = (o["d"] | 0) != 0;
    t.created = o["c"] | 0u;
    t.completed = t.done ? (o["x"] | 0u) : 0u;
    s_t[s_n++] = t;
    if (t.id > maxId) maxId = t.id;
  }
  uint32_t stored = doc["next"] | 0u;
  s_next = max(stored, maxId + 1);
  sortTasks();
  s_ver++;
  Serial.printf("[TODO] loaded %d tasks\n", s_n);
}

int tasks_count() { return s_n; }
const Task* tasks_at(int i) { return (i >= 0 && i < s_n) ? &s_t[i] : nullptr; }
int tasks_active_count() { int c = 0; for (int i = 0; i < s_n; i++) if (!s_t[i].done) c++; return c; }
int tasks_done_count() { return s_n - tasks_active_count(); }
uint32_t tasks_version() { return s_ver; }
bool tasks_save_ok() { return s_saveOk; }

bool tasks_add(const char* text) {
  if (s_n >= SB_MAX_TASKS) return false;
  Task t;
  memset(&t, 0, sizeof(t));
  if (!cleanText(text, t.text)) return false;
  t.id = s_next++;
  t.created = (uint32_t)time_now();
  s_t[s_n++] = t;
  sortTasks();
  changed();
  return true;
}

bool tasks_toggle(uint32_t id) {
  for (int i = 0; i < s_n; i++) {
    if (s_t[i].id != id) continue;
    s_t[i].done = !s_t[i].done;
    s_t[i].completed = s_t[i].done ? (uint32_t)time_now() : 0;
    sortTasks();
    changed();
    return true;
  }
  return false;
}

bool tasks_delete(uint32_t id) {
  for (int i = 0; i < s_n; i++) {
    if (s_t[i].id != id) continue;
    for (int j = i; j < s_n - 1; j++) s_t[j] = s_t[j + 1];
    s_n--;
    changed();
    return true;
  }
  return false;
}

static bool saveNow() {
  JsonDocument doc;
  doc["next"] = s_next;
  JsonArray arr = doc["tasks"].to<JsonArray>();
  for (int i = 0; i < s_n; i++) {
    JsonObject o = arr.add<JsonObject>();
    o["id"] = s_t[i].id;
    o["t"] = s_t[i].text;
    o["d"] = s_t[i].done ? 1 : 0;
    o["c"] = s_t[i].created;
    o["x"] = s_t[i].completed;
  }
  return storage_save_json(SB_FILE_TASKS, doc);
}

static void expire() {
  if (!time_valid()) return;
  uint32_t now = (uint32_t)time_now();
  bool any = false;
  for (int i = s_n - 1; i >= 0; i--) {
    if (s_t[i].done && s_t[i].completed == 0) { s_t[i].completed = now; any = true; continue; }  // stamp late
    if (s_t[i].done && now >= s_t[i].completed && now - s_t[i].completed >= SB_DONE_EXPIRY_SEC) {
      for (int j = i; j < s_n - 1; j++) s_t[j] = s_t[j + 1];
      s_n--; any = true;
    }
  }
  if (any) { sortTasks(); changed(); }
}

void tasks_loop() {
  uint32_t now = millis();
  if (time_valid() && (!s_stamped || now - s_lastExpiry >= 60000UL)) {
    s_stamped = true; s_lastExpiry = now;
    expire();
  }
  if (s_dirty && now - s_dirtyAt >= 600) {
    if (saveNow()) { s_dirty = false; s_saveOk = true; }
    else { s_saveOk = false; s_dirtyAt = now + 8000; }
  }
}
