// Photo / wallpaper file lists on the SD card (sorted, validated names).
#pragma once
#include <Arduino.h>
#include "config.h"

enum MediaKind : uint8_t { MEDIA_IMAGES = 0, MEDIA_BACKGROUNDS = 1 };

const char* media_dir(MediaKind k);
int         media_refresh(MediaKind k);                  // rescan directory, returns count
int         media_count(MediaKind k);
const char* media_name(MediaKind k, int i);
uint32_t    media_size(MediaKind k, int i);
int         media_find(MediaKind k, const char* name);   // exact match in list, -1 if none
bool        media_path(MediaKind k, const char* name, char* out, size_t n);  // only for listed files
bool        media_delete(MediaKind k, const char* name);
uint32_t    media_version(MediaKind k);

// Upload helpers (used by the web server)
bool        media_sanitize_name(const char* raw, char* out, size_t n);   // jpg/jpeg only
bool        media_upload_tmp_path(MediaKind k, char* out, size_t n);
bool        media_upload_commit(MediaKind k, const char* rawName, char* finalName, size_t n);
void        media_upload_discard(MediaKind k);
