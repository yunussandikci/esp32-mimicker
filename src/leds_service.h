#pragma once
#include <Arduino.h>
#include "config.h"
#include "script_service.h"

namespace leds {

namespace detail {

enum Mode {
  BOOTING,
  CONNECTING,
  AP_SETUP,
  STA_READY,
};

inline Mode    mode       = BOOTING;
inline uint8_t last_red   = 255;
inline uint8_t last_green = 255;
inline uint8_t last_blue  = 255;


inline void push(uint8_t red, uint8_t green, uint8_t blue) {
  if (red == last_red && green == last_green && blue == last_blue) {
    return;
  }
  last_red   = red;
  last_green = green;
  last_blue  = blue;
  neopixelWrite(cfg::PIN_RGB, red, green, blue);
}

} // namespace detail


inline void wifi_connecting() {
  detail::mode = detail::CONNECTING;
  const uint8_t bright = cfg::LED_BRIGHTNESS;
  detail::push(bright, bright, 0);
}


inline void wifi_ap_setup() {
  detail::mode = detail::AP_SETUP;
  const uint8_t bright = cfg::LED_BRIGHTNESS;
  detail::push(bright, 0, bright);
}


inline void wifi_ready() {
  detail::mode = detail::STA_READY;
}


inline void loop() {
  if (detail::mode != detail::STA_READY) {
    return;
  }
  const uint8_t bright = cfg::LED_BRIGHTNESS;
  if (script::running) {
    detail::push(0, bright, 0);
  } else {
    detail::push(0, 0, bright);
  }
}

} // namespace leds
