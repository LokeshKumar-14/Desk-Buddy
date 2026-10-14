// Hardware initialisation copied from the working Elecrow LVGL_Arduino7.0 sketch:
// same RGB pin map / timings, same PCA9557 reset sequence, same GT911 touch driver
// (touch.h is the unmodified original), same LovyanGFX frame buffers handed to LVGL.
#include "display_setup.h"
#include "config.h"
#include <Arduino.h>
#include <Wire.h>
#include <PCA9557.h>
#include <lvgl.h>
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <SPI.h>
#include <esp_heap_caps.h>
#include <type_traits>
#include <utility>

class LGFX : public lgfx::LGFX_Device
{
public:
  lgfx::Bus_RGB _bus_instance;
  lgfx::Panel_RGB _panel_instance;

  LGFX()
  {
    auto busConfig = _bus_instance.config();
    busConfig.panel = &_panel_instance;
    const int8_t dataPins[16] = {15, 7, 6, 5, 4, 9, 46, 3, 8, 16, 1, 14, 21, 47, 48, 45};
    memcpy(busConfig.pin_data, dataPins, sizeof(dataPins));
    busConfig.pin_henable = 41;
    busConfig.pin_vsync = 40;
    busConfig.pin_hsync = 39;
    busConfig.pin_pclk = 0;
    busConfig.freq_write = SB_PCLK_HZ;   // 24 MHz in the original sketch
    busConfig.hsync_polarity = 0;
    busConfig.hsync_front_porch = 40;
    busConfig.hsync_pulse_width = 48;
    busConfig.hsync_back_porch = 40;
    busConfig.vsync_polarity = 0;
    busConfig.vsync_front_porch = 1;
    busConfig.vsync_pulse_width = 31;
    busConfig.vsync_back_porch = 13;
    busConfig.pclk_active_neg = 1;
    busConfig.de_idle_high = 0;
    busConfig.pclk_idle_high = 0;
    _bus_instance.config(busConfig);

    auto panelConfig = _panel_instance.config();
    panelConfig.memory_width = 800;
    panelConfig.memory_height = 480;
    panelConfig.panel_width = 800;
    panelConfig.panel_height = 480;
    _panel_instance.config(panelConfig);
    _panel_instance.setBus(&_bus_instance);
    setPanel(&_panel_instance);
  }
};

LGFX lcd;

static constexpr uint32_t screenWidth = 800;
static constexpr uint32_t screenHeight = 480;

SPIClass& spi = SPI;      // kept from the original sketch

#include "touch.h"        // original GT911 driver, unmodified (needs `lcd` above)

// ---------------------------------------------------------------------------
// Frame-buffer hand-off. The original Elecrow sketch calls Bus_RGB::presentFrameBuffer()
// and getFrameBuffer(), which exist only in Elecrow's patched LovyanGFX. Upstream LovyanGFX
// (library manager) does not have them. We detect at compile time which one is installed:
//   * patched library  -> zero-copy: LVGL renders straight into the two RGB buffers (original way)
//   * upstream library -> LVGL renders into small PSRAM line buffers and pushImage() copies them
// Written without `if constexpr` so it also builds on older (C++11) ESP32 cores.
// ---------------------------------------------------------------------------
template <class B>
static auto zcProbe(int) -> decltype(std::declval<B&>().presentFrameBuffer((uint8_t*)0),
                                     std::declval<B&>().getFrameBuffer(0), std::true_type());
template <class B>
static std::false_type zcProbe(long);
typedef decltype(zcProbe<lgfx::Bus_RGB>(0)) ZeroCopyTag;
static constexpr bool kZeroCopy = ZeroCopyTag::value;

template <class B> static bool zcPresent(B& b, uint8_t* p, std::true_type) { return b.presentFrameBuffer(p); }
template <class B> static bool zcPresent(B&, uint8_t*, std::false_type) { return true; }
template <class B> static void* zcGetFb(B& b, int i, std::true_type) { return (void*)b.getFrameBuffer(i); }
template <class B> static void* zcGetFb(B&, int, std::false_type) { return nullptr; }

static void my_disp_flush(lv_display_t *display, const lv_area_t *area, uint8_t *pixelMap)
{
#ifdef SB_DEBUG_FLUSH
  // Shows exactly which screen area LVGL redraws and when (helps find what keeps refreshing).
  Serial.printf("[flush] x%ld..%ld y%ld..%ld at %lu ms\n", (long)area->x1, (long)area->x2,
                (long)area->y1, (long)area->y2, (unsigned long)millis());
#endif
  if (kZeroCopy) {
#ifndef SB_RENDER_FULL
    // DIRECT mode: LVGL calls flush once per dirty area; only present the finished frame.
    if (!lv_display_flush_is_last(display)) { lv_display_flush_ready(display); return; }
#endif
    if (!zcPresent(lcd._bus_instance, pixelMap, ZeroCopyTag())) {
      Serial.println("LovyanGFX VSYNC frame switch timeout");
    }
  } else {
    int32_t w = area->x2 - area->x1 + 1;
    int32_t h = area->y2 - area->y1 + 1;
    (void)h;
    lcd.startWrite();
#if SB_UPSTREAM_DIRECT
    // pixelMap is the full 800x480 render buffer; copy only the finished area, row by row.
    const uint16_t* base = (const uint16_t*)pixelMap;
    for (int32_t y = area->y1; y <= area->y2; y++) {
      lcd.pushImage(area->x1, y, w, 1, (const lgfx::rgb565_t*)(base + (size_t)y * screenWidth + area->x1));
    }
#else
    // Partial buffers copy a compact stripe instead of keeping another full 768 KB
    // framebuffer in PSRAM. On this RGB panel that is usually much steadier.
    lcd.pushImage(area->x1, area->y1, w, h, (const lgfx::rgb565_t*)pixelMap);
#endif
    lcd.endWrite();
  }
  lv_display_flush_ready(display);
}

// Same as the original callback minus the per-touch Serial printing and the
// delay(15) (which stalled every input poll).
static void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
  (void)indev;
  if (touch_has_signal())
  {
    if (touch_touched())
    {
      data->state = LV_INDEV_STATE_PR;
      data->point.x = touch_last_x;
      data->point.y = touch_last_y;
    }
    else if (touch_released())
    {
      data->state = LV_INDEV_STATE_REL;
    }
  }
  else
  {
    data->state = LV_INDEV_STATE_REL;
  }
}

PCA9557 Out;
static uint8_t s_brightness = 100;

// lv_tick_get_cb_t is uint32_t(*)(void); millis() returns unsigned long on some cores.
static uint32_t tickCb() { return (uint32_t)millis(); }

bool display_begin()
{
  Serial.printf("PSRAM: %s, size: %u, free: %u\n",
                psramFound() ? "OK" : "NOT FOUND", ESP.getPsramSize(), ESP.getFreePsram());

  Wire.begin(SB_I2C_SDA, SB_I2C_SCL);

  // The panel controller must leave reset before the RGB bus starts (original sequence).
  Out.reset();
  Out.setMode(IO_OUTPUT);
  Out.setState(IO0, IO_LOW);
  Out.setState(IO1, IO_LOW);
  delay(20);
  Out.setState(IO0, IO_HIGH);
  delay(100);
  Out.setMode(IO1, IO_INPUT);
  Serial.println("PCA9557 init done");

  pinMode(SB_PIN_AUX_OUT, OUTPUT);       // original sketch holds this output low
  digitalWrite(SB_PIN_AUX_OUT, LOW);

  if (!lcd.begin()) {
    Serial.println("lcd.begin() failed! Check Arduino Tools > PSRAM is set to OPI PSRAM.");
    return false;
  }
  Serial.println("LovyanGFX lcd.begin() OK");
  delay(200);

  lv_init();
  lv_tick_set_cb(tickCb);
  delay(100);

  touch_init();
  Serial.println("touch init done");

  lv_display_t *display = lv_display_create(screenWidth, screenHeight);
  lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
  lv_display_set_flush_cb(display, my_disp_flush);

  if (kZeroCopy) {
    // LovyanGFX owns two full-screen RGB buffers; LVGL renders directly into them.
    void* fb0 = zcGetFb(lcd._bus_instance, 0, ZeroCopyTag());
    void* fb1 = zcGetFb(lcd._bus_instance, 1, ZeroCopyTag());
    Serial.printf("Display path: zero-copy (patched LovyanGFX). Buffers %p, %p, free PSRAM: %u\n",
                  fb0, fb1, ESP.getFreePsram());
#ifdef SB_RENDER_FULL
    lv_display_set_buffers(display, fb0, fb1, screenWidth * screenHeight * sizeof(lv_color_t),
                           LV_DISPLAY_RENDER_MODE_FULL);
#else
    lv_display_set_buffers(display, fb0, fb1, screenWidth * screenHeight * sizeof(lv_color_t),
                           LV_DISPLAY_RENDER_MODE_DIRECT);
#endif
  } else {
#if SB_UPSTREAM_DIRECT
    // Upstream LovyanGFX: one full-size render buffer in PSRAM, DIRECT mode, areas copied with pushImage().
    const size_t bytes = screenWidth * screenHeight * sizeof(lv_color_t);
    void* b0 = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!b0) { Serial.println("LVGL buffer allocation failed"); return false; }
    Serial.printf("Display path: pushImage DIRECT (upstream LovyanGFX), free PSRAM: %u\n", ESP.getFreePsram());
    lv_display_set_buffers(display, b0, NULL, bytes, LV_DISPLAY_RENDER_MODE_DIRECT);
#else
    // Upstream LovyanGFX: two small partial-render buffers in PSRAM, copied with pushImage().
    // 24 lines keeps transfers short and leaves PSRAM bandwidth for RGB scanout.
    const size_t bytes = screenWidth * 24 * sizeof(lv_color_t);
    void* b0 = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    void* b1 = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!b0 || !b1) { Serial.println("LVGL buffer allocation failed"); return false; }
    Serial.printf("Display path: pushImage (upstream LovyanGFX), free PSRAM: %u\n", ESP.getFreePsram());
    lv_display_set_buffers(display, b0, b1, bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
#endif
  }

#if SB_DIAG_NO_TOUCH
  Serial.println("DIAG: touch polling disabled (SB_DIAG_NO_TOUCH)");
#else
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);
#endif

  pinMode(SB_TFT_BL, OUTPUT);
  display_set_brightness(s_brightness);
  return true;
}

void display_set_brightness(uint8_t percent)
{
  s_brightness = constrain(percent, (uint8_t)5, (uint8_t)100);
  uint8_t duty = map(s_brightness, 5, 100, 16, 255);
  analogWrite(SB_TFT_BL, duty);
}
