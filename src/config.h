#pragma once
#include <Arduino.h>
#include <IPAddress.h>

namespace cfg {

constexpr const char* WIFI_SSID      = "SANDIKCI_UST_2.4G";
constexpr const char* WIFI_PASS      = "Selam123.";
constexpr bool        USE_STATIC_IP  = true;
constexpr int         WIFI_TIMEOUT_S = 60;
constexpr uint16_t    HTTP_PORT      = 80;

inline const IPAddress IP_LOCAL  (192, 168, 1, 131);
inline const IPAddress IP_GATEWAY(192, 168, 1, 1);
inline const IPAddress IP_SUBNET (255, 255, 255, 0);

// Named HID_* to dodge the USB_VID/USB_PID macros in pins_arduino.h.
constexpr uint16_t    HID_VID     = 0x046D;
constexpr uint16_t    HID_PID     = 0xC33A;
constexpr const char* HID_PRODUCT = "G413 Gaming Keyboard";
constexpr const char* HID_VENDOR  = "Logitech";
constexpr const char* HID_SERIAL  = "0000314159";

constexpr int     PIN_RGB        = 21;
constexpr int     PIN_TOUCH      = 7;
constexpr uint8_t LED_BRIGHTNESS = 30;

// ESP32-S3 touchRead returns raw counts; touching raises the value.
// We auto-calibrate at boot and trigger when value > baseline * MULT.
constexpr float         TOUCH_TRIGGER_MULT = 1.25f;
constexpr unsigned long TOUCH_DEBOUNCE_MS  = 400;

constexpr size_t MAX_SCRIPT = 4096;

} // namespace cfg
