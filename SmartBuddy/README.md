# Smart Buddy - Elecrow CrowPanel 7.0" (ESP32-S3, 800x480)

Glass-style smart display: clock, weather, calendar, to-do (phone controlled), photo gallery,
wallpapers, timer, themes, Wi-Fi manager and a phone web interface served by the ESP32 itself.

> **Status:** written against the source in your `LVGL_Arduino7_0.zip` (LVGL **9.1.0**, LovyanGFX RGB bus,
> GT911 touch, PCA9557). All modules were syntax-checked on a PC against the real LVGL 9.1.0 headers
> using Arduino stubs. **It has NOT been compiled with the ESP32 toolchain or run on your board** -
> expect to fix a few small things on first build, and test every feature on hardware.

## 1. Arduino IDE setup
Board: **ESP32S3 Dev Module** (esp32 by Espressif core, 3.x recommended)
- Flash Size: **4MB (32Mb)**  (never 16MB)
- PSRAM: **OPI PSRAM**
- Partition Scheme: **Huge APP (3MB No OTA/1MB SPIFFS)** (default 4MB scheme may be too small)
- CPU 240 MHz, otherwise same settings as your working Elecrow example.

Libraries (Library Manager unless noted):
| Library | Version | Note |
|---|---|---|
| lvgl | **9.1.0** | same as the Elecrow project |
| LovyanGFX | the one you already use | provides `presentFrameBuffer` |
| PCA9557 | Elecrow's copy | already installed |
| TAMC_GT911 | already installed | used by `touch.h` |
| **JPEGDEC** (Larry Bank) | 1.8.x | new - baseline JPEG decoder |
| **ArduinoJson** | 7.x | new |

`lv_conf.h`: copy `lvgl_config/lv_conf.h` over the one next to the `lvgl` folder in
`Documents/Arduino/libraries/` (keep a backup of yours). It enables Montserrat 12-48, 16-bit colour and
the C `malloc` (so big UI objects go to PSRAM).

Open `SmartBuddy/SmartBuddy.ino` and upload.

## 2. SD card
FAT32, SPI pins from the Elecrow SD lesson: MOSI 11, MISO 13, SCK 12, CS 10 (`config.h`).
```
/SmartBuddy/images/        gallery photos (.jpg/.jpeg)
/SmartBuddy/backgrounds/   wallpapers (.jpg/.jpeg)
/SmartBuddy/data/          tasks.json, settings.json (+ .bak safety copies)
```
Folders are created automatically. Only **baseline JPEG** is supported (not progressive, PNG or HEIC).
Photos uploaded from the phone page are converted to baseline JPEG in the browser automatically
(wallpapers 800x480, gallery max 1280x800). Files copied manually should be baseline JPEG.

## 3. Wi-Fi and security
- First boot uses `secrets.h` (`Lokiii` / test password). Replace it before sharing the code; `secrets.h`
  is in `.gitignore`, `secrets.example.h` is the template.
- On the display: WIFI button -> scan / type SSID + password -> Connect. Saved in ESP32 NVS flash.
  **NVS is plaintext** unless you enable flash encryption.
- The password is never printed to Serial and never returned by any web endpoint.
- No access point is created. The web server only starts after Wi-Fi gets an IP.
- The web UI has **no login** - use it on your own network only, never port-forward it.
- Weather uses HTTPS but with `setInsecure()` (encrypted, server certificate NOT verified); see
  `SB_TLS_INSECURE` in `config.h` to pin a CA instead. Only weather/location data goes over it.

## 4. Using it
Phone and ESP32 on the same network. Open the IP shown in the WIFI screen / To-Do panel
(or `http://smartbuddy.local/`). Tabs: Home, To-Do, Weather, Photos, Walls, Timer, Settings.
- Upload: choose an image -> **Set as Background** or **Add to Gallery**.
- Tasks: completed tasks sink below active ones, are struck through, and vanish 12 h after completion
  (needs a synced clock; tasks completed before the first NTP sync are stamped when time arrives).
- Weather: Open-Meteo (no API key). Enter a city on the phone; refreshes every 20 min; shows STALE/OFFLINE.
- Default time zone is India (IST-5:30) - change in Settings. The clock shows `--:--` until NTP sync.

## 5. Differences from the original Elecrow sketch
- Display/PCA9557/touch init and pin map are unchanged; `touch.h` is byte-identical.
- With Elecrow's patched LovyanGFX, LVGL uses the original zero-copy RGB buffers.
- With upstream LovyanGFX, LVGL uses small partial buffers and a 12 MHz RGB pixel clock to reduce PSRAM/RGB scan contention.
  Define `SB_RENDER_FULL` in `config.h` to go back to the original FULL mode.
- Touch callback: removed per-touch Serial prints and `delay(15)`.
- DHT20 sensor code removed (not needed). GPIO38 is still driven LOW like the original.

## 6. Known limitations
- Fonts are LVGL Montserrat: Latin text only (Hinglish in Roman letters works; Devanagari does not).
- SD reads/JPEG decode and uploads run on the main loop, so the UI pauses ~0.5-1.5 s when a wallpaper or
  photo loads.
- Calendar week starts Monday (`SB_WEEK_STARTS_MONDAY`), clock 24 h (`SB_CLOCK_24H`).
- The request text you sent was cut off at "14. MEMORY AND PERFORMANCE"; sections after it were not
  seen, so any requirements there are not covered.

## 7. Files
`SmartBuddy.ino` main loop | `display_setup.*` hardware + LVGL bring-up | `storage_manager.*` SD + safe JSON |
`settings_manager.*` | `task_manager.*` | `timer_manager.*` | `time_manager.*` | `wifi_manager.*` |
`weather_manager.*` (background task) | `media_manager.*`, `image_loader.*`, `wallpaper_manager.*` |
`theme_manager.*` | `ui_*.cpp` screens | `web_server.*` + `web_page.h` (generated from `web_src/index.html`
with `python tools/make_web_page.py`).
