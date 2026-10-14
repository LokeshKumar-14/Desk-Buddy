// Phone web interface (HTTP on port 80, runs on the ESP32 itself).
#pragma once
void web_begin();     // call once, after Wi-Fi has an IP
void web_loop();      // call every loop()
bool web_running();
