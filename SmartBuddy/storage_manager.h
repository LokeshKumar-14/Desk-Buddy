// SD card access: mount, safe JSON load/save, directory helpers.
// Everything that touches the SD card runs on the Arduino loop() task only.
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

enum SdState : uint8_t { SD_ST_UNKNOWN = 0, SD_ST_OK, SD_ST_MISSING, SD_ST_ERROR };

bool      storage_begin();                 // mount + create /SmartBuddy tree
void      storage_loop();                  // periodic presence check / remount
bool      storage_ok();
SdState   storage_state();
uint64_t  storage_total_bytes();
uint64_t  storage_free_bytes();            // cached ~30 s
bool      storage_ensure_dir(const char* path);

// Loads path (falls back to path + ".bak"). Returns false if neither parses.
bool      storage_load_json(const char* path, JsonDocument& doc);
// Serialises and atomically replaces path (tmp file + .bak). Skips the write
// when the content is identical to what was last loaded/saved.
bool      storage_save_json(const char* path, const JsonDocument& doc);
