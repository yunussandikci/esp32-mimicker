#pragma once
#include <Arduino.h>

namespace cfg {

constexpr const char*   AP_SSID           = "ESP32-Mimicker";
constexpr int           WIFI_TIMEOUT_S    = 15;
constexpr unsigned long WIFI_DROP_WIPE_MS = 30000;
constexpr uint16_t      HTTP_PORT         = 80;

// Named HID_* to dodge the USB_VID/USB_PID macros in pins_arduino.h.
constexpr uint16_t    HID_VID     = 0x046D;
constexpr uint16_t    HID_PID     = 0xC33A;
constexpr const char* HID_PRODUCT = "G413 Gaming Keyboard";
constexpr const char* HID_VENDOR  = "Logitech";
constexpr const char* HID_SERIAL  = "0000314159";

constexpr int     PIN_RGB        = 21;
constexpr uint8_t LED_BRIGHTNESS = 30;

constexpr size_t MAX_SCRIPT = 4096;

} // namespace cfg
