#pragma once
#include <Arduino.h>
#include "config.h"
#include "script_service.h"

namespace leds {

namespace detail {

inline bool    track_script = false;
inline uint8_t last_red     = 255;
inline uint8_t last_green   = 255;
inline uint8_t last_blue    = 255;


inline void push(uint8_t red, uint8_t green, uint8_t blue) {
  if (red == last_red && green == last_green && blue == last_blue) return;
  last_red = red; last_green = green; last_blue = blue;
  neopixelWrite(cfg::PIN_RGB, red, green, blue);
}

} // namespace detail


inline void wifi_connecting() {
  detail::track_script = false;
  const uint8_t b = cfg::LED_BRIGHTNESS;
  detail::push(b, b, 0);
}


inline void wifi_ap_setup() {
  detail::track_script = false;
  const uint8_t b = cfg::LED_BRIGHTNESS;
  detail::push(b, 0, b);
}


inline void wifi_ready() {
  detail::track_script = true;
}


inline void loop() {
  if (!detail::track_script) return;
  const uint8_t b = cfg::LED_BRIGHTNESS;
  if (script::running) detail::push(0, b, 0);
  else                 detail::push(0, 0, b);
}

} // namespace leds
