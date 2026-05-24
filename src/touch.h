#pragma once
#include <Arduino.h>
#include "config.h"
#include "script.h"

namespace touch {

namespace detail {

inline uint32_t      threshold      = 0;
inline bool          was_touched    = false;
inline unsigned long last_toggle_ms = 0;

} // namespace detail


inline void init() {
  uint32_t sum = 0;
  for (int sample = 0; sample < 16; sample++) {
    sum += touchRead(cfg::PIN_TOUCH);
    delay(10);
  }
  uint32_t baseline = sum / 16;
  detail::threshold = (uint32_t)(baseline * cfg::TOUCH_TRIGGER_MULT);
}


inline void loop() {
  bool          is_touched   = touchRead(cfg::PIN_TOUCH) > detail::threshold;
  unsigned long now_ms       = millis();
  bool          rising_edge  = is_touched && !detail::was_touched;
  bool          past_debounce = (now_ms - detail::last_toggle_ms) > cfg::TOUCH_DEBOUNCE_MS;

  if (rising_edge && past_debounce) {
    script::toggle();
    detail::last_toggle_ms = now_ms;
  }

  detail::was_touched = is_touched;
}

} // namespace touch
