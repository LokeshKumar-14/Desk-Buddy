#include "image_loader.h"
#include "config.h"
#include <JPEGDEC.h>
#include <SD.h>
#include <esp_heap_caps.h>

void img_init(SbImage* im) { memset(im, 0, sizeof(*im)); }

void img_free(SbImage* im) {
  if (im->pixels) heap_caps_free(im->pixels);
  img_init(im);
}

static bool allocImage(SbImage* im, uint16_t w, uint16_t h) {
  img_init(im);
  size_t bytes = (size_t)w * h * 2;
  im->pixels = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!im->pixels) return false;
  im->w = w; im->h = h;
  im->dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
  im->dsc.header.cf = LV_COLOR_FORMAT_RGB565;
  im->dsc.header.flags = 0;
  im->dsc.header.w = w;
  im->dsc.header.h = h;
  im->dsc.header.stride = w * 2;
  im->dsc.data_size = (uint32_t)bytes;
  im->dsc.data = (const uint8_t*)im->pixels;
  return true;
}

// --------------------------------------------------------------- decoding
struct DecCtx {
  uint16_t* dst;
  int dstW, dstH;
  int cropX, cropY, cropW, cropH;        // in decoder (scaled) coordinates
  uint16_t* mapX;                        // dest col -> source col
  uint16_t* mapY;                        // dest row -> source row
};

static inline int ceilDiv(int a, int b) { return (a + b - 1) / b; }

static int drawCb(JPEGDRAW* d) {
  DecCtx* c = (DecCtx*)d->pUser;
  int bx0 = d->x, by0 = d->y, bw = d->iWidthUsed, bh = d->iHeight;
  int sx0 = max(bx0, c->cropX), sx1 = min(bx0 + bw, c->cropX + c->cropW);
  int sy0 = max(by0, c->cropY), sy1 = min(by0 + bh, c->cropY + c->cropH);
  if (sx0 >= sx1 || sy0 >= sy1) return 1;
  int dx0 = ceilDiv((sx0 - c->cropX) * c->dstW, c->cropW);
  int dx1 = ceilDiv((sx1 - c->cropX) * c->dstW, c->cropW);
  int dy0 = ceilDiv((sy0 - c->cropY) * c->dstH, c->cropH);
  int dy1 = ceilDiv((sy1 - c->cropY) * c->dstH, c->cropH);
  if (dx1 > c->dstW) dx1 = c->dstW;
  if (dy1 > c->dstH) dy1 = c->dstH;
  for (int dy = dy0; dy < dy1; dy++) {
    const uint16_t* src = d->pPixels + (size_t)(c->mapY[dy] - by0) * d->iWidth;
    uint16_t* out = c->dst + (size_t)dy * c->dstW;
    for (int dx = dx0; dx < dx1; dx++) out[dx] = src[c->mapX[dx] - bx0];
  }
  return 1;
}

bool img_load_jpeg(const char* path, uint16_t maxW, uint16_t maxH, FitMode mode,
                   SbImage* out, const char** err) {
  img_init(out);
  const char* e = nullptr;
  File f = SD.open(path, FILE_READ);
  if (!f) { if (err) *err = "cannot open file"; return false; }

  JPEGDEC* jpg = new (std::nothrow) JPEGDEC();
  DecCtx ctx;
  memset(&ctx, 0, sizeof(ctx));
  bool ok = false;
  if (!jpg) { e = "out of memory"; goto done; }
  if (!jpg->open(f, drawCb)) { e = "not a supported JPEG (baseline only)"; goto done; }
  {
    int W = jpg->getWidth(), H = jpg->getHeight();
    if (W < 8 || H < 8 || W > SB_JPEG_MAX_DIM || H > SB_JPEG_MAX_DIM) { e = "bad image size"; goto done; }
    int dstW, dstH, cropX = 0, cropY = 0, cropW = W, cropH = H;
    if (mode == FIT_COVER) {
      dstW = maxW; dstH = maxH;
      if ((long)W * maxH > (long)H * maxW) { cropW = (int)((long)H * maxW / maxH); cropX = (W - cropW) / 2; }
      else { cropH = (int)((long)W * maxH / maxW); cropY = (H - cropH) / 2; }
    } else {
      if (W <= maxW && H <= maxH) { dstW = W; dstH = H; }
      else if ((long)W * maxH > (long)H * maxW) { dstW = maxW; dstH = (int)((long)H * maxW / W); }
      else { dstH = maxH; dstW = (int)((long)W * maxH / H); }
      if (dstW < 1) dstW = 1;
      if (dstH < 1) dstH = 1;
    }
    // largest hardware down-scale that still keeps >= dest resolution
    int div = 1, opt = 0;
    if (cropW / 8 >= dstW && cropH / 8 >= dstH) { div = 8; opt = JPEG_SCALE_EIGHTH; }
    else if (cropW / 4 >= dstW && cropH / 4 >= dstH) { div = 4; opt = JPEG_SCALE_QUARTER; }
    else if (cropW / 2 >= dstW && cropH / 2 >= dstH) { div = 2; opt = JPEG_SCALE_HALF; }
    ctx.cropX = cropX / div; ctx.cropY = cropY / div;
    ctx.cropW = max(1, cropW / div); ctx.cropH = max(1, cropH / div);
    ctx.dstW = dstW; ctx.dstH = dstH;

    if (!allocImage(out, dstW, dstH)) { e = "not enough PSRAM"; goto done; }
    ctx.dst = out->pixels;
    ctx.mapX = (uint16_t*)malloc(sizeof(uint16_t) * dstW);
    ctx.mapY = (uint16_t*)malloc(sizeof(uint16_t) * dstH);
    if (!ctx.mapX || !ctx.mapY) { e = "out of memory"; goto done; }
    for (int x = 0; x < dstW; x++) ctx.mapX[x] = ctx.cropX + (x * ctx.cropW) / dstW;
    for (int y = 0; y < dstH; y++) ctx.mapY[y] = ctx.cropY + (y * ctx.cropH) / dstH;
    memset(out->pixels, 0, (size_t)dstW * dstH * 2);

    jpg->setPixelType(RGB565_LITTLE_ENDIAN);
    jpg->setUserPointer(&ctx);
    ok = jpg->decode(0, 0, opt) != 0;
    if (!ok) e = "decode failed (corrupt or progressive JPEG)";
  }
done:
  if (jpg) { jpg->close(); delete jpg; }
  f.close();
  if (ctx.mapX) free(ctx.mapX);
  if (ctx.mapY) free(ctx.mapY);
  if (!ok) img_free(out);
  if (err) *err = e;
  return ok;
}

// --------------------------------------------------------------- default wallpaper
struct Blob { int cx, cy, r; uint8_t cr, cg, cb; uint8_t k; };

bool img_make_default_wallpaper(uint8_t themeIdx, SbImage* out) {
  if (!allocImage(out, SB_SCREEN_W, SB_SCREEN_H)) return false;
  const bool dark = (themeIdx == 0);
  const uint8_t c0[3] = { (uint8_t)(dark ? 0x09 : 0xD8), (uint8_t)(dark ? 0x0E : 0xE6), (uint8_t)(dark ? 0x22 : 0xFF) };
  const uint8_t c1[3] = { (uint8_t)(dark ? 0x22 : 0xF3), (uint8_t)(dark ? 0x14 : 0xDF), (uint8_t)(dark ? 0x4A : 0xFF) };
  const Blob b[3] = {
    { 170, 130, 330, (uint8_t)(dark ? 0x2A : 0x7F), (uint8_t)(dark ? 0x5C : 0xB2), (uint8_t)(dark ? 0xFF : 0xFF), 170 },
    { 650, 380, 380, (uint8_t)(dark ? 0xA0 : 0xFF), (uint8_t)(dark ? 0x3A : 0xA6), (uint8_t)(dark ? 0xFF : 0xD9), 150 },
    { 700,  80, 240, (uint8_t)(dark ? 0x00 : 0x9F), (uint8_t)(dark ? 0xB8 : 0xF0), (uint8_t)(dark ? 0xAE : 0xD0), 120 },
  };
  for (int y = 0; y < SB_SCREEN_H; y++) {
    uint16_t* row = out->pixels + (size_t)y * SB_SCREEN_W;
    for (int x = 0; x < SB_SCREEN_W; x++) {
      int t = (x * 255 / SB_SCREEN_W + y * 255 / SB_SCREEN_H) >> 1;
      int r = c0[0] + ((c1[0] - c0[0]) * t) / 255;
      int g = c0[1] + ((c1[1] - c0[1]) * t) / 255;
      int bl = c0[2] + ((c1[2] - c0[2]) * t) / 255;
      for (int i = 0; i < 3; i++) {
        int dx = x - b[i].cx, dy = y - b[i].cy;
        int d2 = dx * dx + dy * dy, r2 = b[i].r * b[i].r;
        if (d2 >= r2) continue;
        int w = ((r2 - d2) * 255) / r2;
        w = (w * w) >> 8;
        w = (w * b[i].k) >> 8;
        r += ((b[i].cr - r) * w) >> 8;
        g += ((b[i].cg - g) * w) >> 8;
        bl += ((b[i].cb - bl) * w) >> 8;
      }
      row[x] = (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (bl >> 3));
    }
  }
  return true;
}

bool img_make_solid_wallpaper(uint32_t rgb, SbImage* out) {
  if (!allocImage(out, SB_SCREEN_W, SB_SCREEN_H)) return false;
  int r0 = (rgb >> 16) & 0xFF;
  int g0 = (rgb >> 8) & 0xFF;
  int b0 = rgb & 0xFF;
  for (int y = 0; y < SB_SCREEN_H; y++) {
    uint16_t* row = out->pixels + (size_t)y * SB_SCREEN_W;
    for (int x = 0; x < SB_SCREEN_W; x++) {
      int shade = 18 - (x * 10 / SB_SCREEN_W) - (y * 8 / SB_SCREEN_H);
      int r = constrain(r0 + shade, 0, 255);
      int g = constrain(g0 + shade, 0, 255);
      int b = constrain(b0 + shade, 0, 255);
      row[x] = (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
    }
  }
  return true;
}
