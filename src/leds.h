#pragma once
#include <Arduino.h>
#include "config.h"
#include "script.h"

namespace leds {

namespace detail {

inline bool    wifi_ok    = false;
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
  const uint8_t bright = cfg::LED_BRIGHTNESS;
  detail::push(bright, bright, 0);
}


inline void wifi_ready() {
  detail::wifi_ok = true;
}


inline void wifi_failed() {
  detail::wifi_ok = false;
  const uint8_t bright = cfg::LED_BRIGHTNESS;
  detail::push(bright, 0, 0);
}


inline void loop() {
  if (!detail::wifi_ok) {
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
