#include "media_manager.h"
#include "storage_manager.h"
#include <SD.h>
#include <FS.h>

struct MediaList {
  char     names[SB_MAX_MEDIA_FILES][SB_MEDIA_NAME_MAX];
  uint32_t sizes[SB_MAX_MEDIA_FILES];
  int      count;
  uint32_t ver;
};
static MediaList s_l[2];

const char* media_dir(MediaKind k) { return k == MEDIA_IMAGES ? SB_DIR_IMAGES : SB_DIR_BG; }
int media_count(MediaKind k) { return s_l[k].count; }
const char* media_name(MediaKind k, int i) { return (i >= 0 && i < s_l[k].count) ? s_l[k].names[i] : ""; }
uint32_t media_size(MediaKind k, int i) { return (i >= 0 && i < s_l[k].count) ? s_l[k].sizes[i] : 0; }
uint32_t media_version(MediaKind k) { return s_l[k].ver; }

static bool hasJpegExt(const char* n) {
  const char* dot = strrchr(n, '.');
  if (!dot) return false;
  return !strcasecmp(dot, ".jpg") || !strcasecmp(dot, ".jpeg");
}

static const char* baseName(const char* n) {
  const char* s = strrchr(n, '/');
  return s ? s + 1 : n;
}

int media_refresh(MediaKind k) {
  MediaList& L = s_l[k];
  L.count = 0;
  L.ver++;
  if (!storage_ok()) return 0;
  File dir = SD.open(media_dir(k));
  if (!dir || !dir.isDirectory()) { if (dir) dir.close(); return 0; }
  File f = dir.openNextFile();
  while (f && L.count < SB_MAX_MEDIA_FILES) {
    if (!f.isDirectory()) {
      const char* nm = baseName(f.name());
      size_t len = strlen(nm);
      if (nm[0] != '.' && len > 0 && len < SB_MEDIA_NAME_MAX && hasJpegExt(nm) && f.size() > 0) {
        // insertion sort, case-insensitive
        int pos = L.count;
        while (pos > 0 && strcasecmp(nm, L.names[pos - 1]) < 0) {
          strcpy(L.names[pos], L.names[pos - 1]);
          L.sizes[pos] = L.sizes[pos - 1];
          pos--;
        }
        strcpy(L.names[pos], nm);
        L.sizes[pos] = (uint32_t)f.size();
        L.count++;
      }
    }
    f.close();
    f = dir.openNextFile();
  }
  if (f) f.close();
  dir.close();
  return L.count;
}

int media_find(MediaKind k, const char* name) {
  if (!name || !name[0]) return -1;
  for (int i = 0; i < s_l[k].count; i++) if (!strcmp(s_l[k].names[i], name)) return i;
  return -1;
}

bool media_path(MediaKind k, const char* name, char* out, size_t n) {
  if (media_find(k, name) < 0) return false;
  int w = snprintf(out, n, "%s/%s", media_dir(k), name);
  return w > 0 && (size_t)w < n;
}

bool media_delete(MediaKind k, const char* name) {
  char p[128];
  if (!media_path(k, name, p, sizeof(p))) return false;
  bool ok = SD.remove(p);
  media_refresh(k);
  return ok;
}

// --------------------------------------------------------------- upload
bool media_sanitize_name(const char* raw, char* out, size_t n) {
  if (!raw || n < 8) return false;
  const char* b = raw;
  for (const char* p = raw; *p; p++) if (*p == '/' || *p == '\\') b = p + 1;
  char tmp[96];
  size_t t = 0;
  for (; *b && t < sizeof(tmp) - 1; b++) {
    unsigned char c = (unsigned char)*b;
    if (isalnum(c) || c == '.' || c == '-' || c == '_') tmp[t++] = (char)c;
    else if (c == ' ') tmp[t++] = '_';
  }
  tmp[t] = 0;
  char* s = tmp;
  while (*s == '.') s++;                       // no hidden / dot-dot names
  if (!hasJpegExt(s)) return false;
  char* dot = strrchr(s, '.');
  size_t extLen = strlen(dot);
  size_t baseLen = (size_t)(dot - s);
  if (baseLen == 0) return false;
  size_t maxBase = n - 1 - extLen - 4;         // leave room for _NN suffix
  if (baseLen > maxBase) baseLen = maxBase;
  memcpy(out, s, baseLen);
  strcpy(out + baseLen, dot);
  return true;
}

bool media_upload_tmp_path(MediaKind k, char* out, size_t n) {
  int w = snprintf(out, n, "%s/.upload.tmp", media_dir(k));
  return w > 0 && (size_t)w < n;
}

void media_upload_discard(MediaKind k) {
  char p[96];
  if (media_upload_tmp_path(k, p, sizeof(p)) && SD.exists(p)) SD.remove(p);
}

bool media_upload_commit(MediaKind k, const char* rawName, char* finalName, size_t n) {
  char clean[SB_MEDIA_NAME_MAX];
  if (!media_sanitize_name(rawName, clean, sizeof(clean))) { media_upload_discard(k); return false; }
  char dst[128], tmp[96];
  media_upload_tmp_path(k, tmp, sizeof(tmp));
  char* dot = strrchr(clean, '.');
  char base[SB_MEDIA_NAME_MAX];
  size_t bl = dot - clean;
  memcpy(base, clean, bl); base[bl] = 0;
  char ext[8]; strlcpy(ext, dot, sizeof(ext));
  strlcpy(finalName, clean, n);
  for (int i = 1; i < 100; i++) {
    snprintf(dst, sizeof(dst), "%s/%s", media_dir(k), finalName);
    if (!SD.exists(dst)) break;
    snprintf(finalName, n, "%s_%d%s", base, i, ext);
  }
  snprintf(dst, sizeof(dst), "%s/%s", media_dir(k), finalName);
  if (SD.exists(dst)) { media_upload_discard(k); return false; }
  bool ok = SD.rename(tmp, dst);
  if (!ok) media_upload_discard(k);
  media_refresh(k);
  return ok;
}
