#include "storage_manager.h"
#include "config.h"
#include <SPI.h>
#include <SD.h>
#include <FS.h>

static SdState  s_state = SD_ST_UNKNOWN;
static uint32_t s_lastCheck = 0;
static uint64_t s_freeCache = 0;
static uint32_t s_freeAt = 0;

// last-written content hash per file (avoid rewriting identical data)
struct HashEntry { uint32_t pathHash; uint32_t contentHash; };
static HashEntry s_hashes[6];

static uint32_t fnv(const uint8_t* d, size_t n, uint32_t h = 2166136261u) {
  for (size_t i = 0; i < n; i++) { h ^= d[i]; h *= 16777619u; }
  return h;
}
static HashEntry* hashFor(const char* path) {
  uint32_t ph = fnv((const uint8_t*)path, strlen(path));
  for (auto& e : s_hashes) if (e.pathHash == ph) return &e;
  for (auto& e : s_hashes) if (e.pathHash == 0) { e.pathHash = ph; e.contentHash = 0; return &e; }
  return &s_hashes[0];
}

static bool mountCard() {
  SD.end();
  SPI.begin(SB_SD_SCK, SB_SD_MISO, SB_SD_MOSI);
  bool ok = SD.begin(SB_SD_CS, SPI, SB_SD_FREQ_HZ);
  if (!ok) { SD.end(); ok = SD.begin(SB_SD_CS, SPI, 4000000UL); }
  if (!ok) { s_state = SD_ST_MISSING; return false; }
  if (SD.cardType() == CARD_NONE) { SD.end(); s_state = SD_ST_MISSING; return false; }
  s_state = SD_ST_OK;
  return true;
}

bool storage_ensure_dir(const char* path) {
  if (SD.exists(path)) return true;
  return SD.mkdir(path);
}

bool storage_begin() {
#if SB_DIAG_NO_SD
  s_state = SD_ST_MISSING;
  Serial.println("DIAG: SD disabled (SB_DIAG_NO_SD)");
  return false;
#endif
  if (!mountCard()) { Serial.println("[SD] mount failed / no card"); return false; }
  Serial.printf("[SD] mounted, %llu MB\n", SD.cardSize() / (1024ULL * 1024ULL));
  const char* dirs[] = { SB_DIR_ROOT, SB_DIR_IMAGES, SB_DIR_BG, SB_DIR_DATA };
  for (auto d : dirs) {
    if (!storage_ensure_dir(d)) { Serial.printf("[SD] cannot create %s\n", d); s_state = SD_ST_ERROR; return false; }
  }
  return true;
}

bool storage_ok() { return s_state == SD_ST_OK; }
SdState storage_state() { return s_state; }

void storage_loop() {
#if SB_DIAG_NO_SD
  return;
#endif
  uint32_t now = millis();
  if (now - s_lastCheck < (s_state == SD_ST_OK ? 10000UL : 5000UL)) return;
  s_lastCheck = now;
  if (s_state == SD_ST_OK) {
    File f = SD.open(SB_DIR_ROOT);
    if (!f || !f.isDirectory()) { if (f) f.close(); s_state = SD_ST_ERROR; Serial.println("[SD] card lost"); }
    else f.close();
  } else {
    if (storage_begin()) Serial.println("[SD] card restored");
  }
}

uint64_t storage_total_bytes() { return storage_ok() ? SD.totalBytes() : 0; }

uint64_t storage_free_bytes() {
  if (!storage_ok()) return 0;
  uint32_t now = millis();
  if (s_freeAt == 0 || now - s_freeAt > 30000UL) {
    uint64_t t = SD.totalBytes(), u = SD.usedBytes();
    s_freeCache = (t > u) ? (t - u) : 0;
    s_freeAt = now ? now : 1;
  }
  return s_freeCache;
}

static bool readFileText(const char* path, char** out, size_t* len, size_t maxLen) {
  File f = SD.open(path, FILE_READ);
  if (!f || f.isDirectory()) { if (f) f.close(); return false; }
  size_t sz = f.size();
  if (sz == 0 || sz > maxLen) { f.close(); return false; }
  char* buf = (char*)malloc(sz + 1);
  if (!buf) { f.close(); return false; }
  size_t got = f.read((uint8_t*)buf, sz);
  f.close();
  if (got != sz) { free(buf); return false; }
  buf[sz] = 0;
  *out = buf; *len = sz;
  return true;
}

bool storage_load_json(const char* path, JsonDocument& doc) {
  if (!storage_ok()) return false;
  char bak[96];
  snprintf(bak, sizeof(bak), "%s.bak", path);
  const char* tries[2] = { path, bak };
  for (int i = 0; i < 2; i++) {
    char* buf = nullptr; size_t len = 0;
    if (!readFileText(tries[i], &buf, &len, 64 * 1024)) continue;
    doc.clear();
    DeserializationError err = deserializeJson(doc, buf, len);
    if (!err) {
      hashFor(path)->contentHash = fnv((const uint8_t*)buf, len);
      free(buf);
      if (i == 1) Serial.printf("[SD] %s recovered from backup\n", path);
      return true;
    }
    Serial.printf("[SD] %s corrupt (%s)\n", tries[i], err.c_str());
    free(buf);
  }
  doc.clear();
  return false;
}

bool storage_save_json(const char* path, const JsonDocument& doc) {
  if (!storage_ok()) return false;
  size_t need = measureJson(doc);
  if (need == 0 || need > 64 * 1024) return false;
  char* buf = (char*)malloc(need + 1);
  if (!buf) return false;
  size_t n = serializeJson(doc, buf, need + 1);
  uint32_t h = fnv((const uint8_t*)buf, n);
  HashEntry* he = hashFor(path);
  if (he->contentHash == h && SD.exists(path)) { free(buf); return true; }   // unchanged

  if (storage_free_bytes() < need + 65536ULL) { free(buf); Serial.println("[SD] not enough space"); return false; }

  char tmp[96], bak[96];
  snprintf(tmp, sizeof(tmp), "%s.tmp", path);
  snprintf(bak, sizeof(bak), "%s.bak", path);
  File f = SD.open(tmp, FILE_WRITE);
  if (!f) { free(buf); return false; }
  size_t w = f.write((const uint8_t*)buf, n);
  f.flush();
  f.close();
  free(buf);
  if (w != n) { SD.remove(tmp); return false; }
  // verify what landed on the card
  File v = SD.open(tmp, FILE_READ);
  bool sizeOk = v && (size_t)v.size() == n;
  if (v) v.close();
  if (!sizeOk) { SD.remove(tmp); return false; }

  if (SD.exists(bak)) SD.remove(bak);
  if (SD.exists(path)) {
    if (!SD.rename(path, bak)) { SD.remove(tmp); return false; }
  }
  if (!SD.rename(tmp, path)) {
    SD.rename(bak, path);            // roll back
    return false;
  }
  he->contentHash = h;
  return true;
}
