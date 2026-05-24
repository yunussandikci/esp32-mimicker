#pragma once
#include <Arduino.h>
#include <USBHIDKeyboard.h>
#include <USBHIDMouse.h>
#include "config.h"

namespace script {

inline char text[cfg::MAX_SCRIPT] =
  "// HID Mimicker Script\r\n"
  "// Commands: WAIT KEY KEYDOWN KEYUP STRING STRINGLN\r\n"
  "//           MOUSE WHEEL CLICK PRESS RELEASE REPEAT\r\n"
  "\r\n"
  "WAIT 3000\r\n"
  "STRING Hello from ESP32-S3 HID!\r\n"
  "KEY ENTER\r\n";

inline volatile bool running = false;


namespace detail {

inline USBHIDKeyboard keyboard;
inline USBHIDMouse    mouse;

struct KeyName {
  const char* name;
  uint8_t     code;
};

inline const KeyName KEYS[] = {
  {"BACKSPACE", KEY_BACKSPACE}, {"CAPSLOCK", KEY_CAPS_LOCK},
  {"DELETE",    KEY_DELETE},    {"INSERT",   KEY_INSERT},
  {"PGDOWN",    KEY_PAGE_DOWN}, {"PGUP",     KEY_PAGE_UP},
  {"RETURN",    KEY_RETURN},    {"ENTER",    KEY_RETURN},
  {"SHIFT",     KEY_LEFT_SHIFT},{"CTRL",     KEY_LEFT_CTRL}, {"CONTROL", KEY_LEFT_CTRL},
  {"ALT",       KEY_LEFT_ALT},  {"GUI",      KEY_LEFT_GUI},  {"WINDOWS", KEY_LEFT_GUI},
  {"TAB",       KEY_TAB},       {"ESC",      KEY_ESC},       {"ESCAPE",  KEY_ESC},
  {"SPACE",     ' '},           {"HOME",     KEY_HOME},      {"END",     KEY_END},
  {"UP",        KEY_UP_ARROW},  {"DOWN",     KEY_DOWN_ARROW},
  {"LEFT",      KEY_LEFT_ARROW},{"RIGHT",    KEY_RIGHT_ARROW},
  {"F1", KEY_F1}, {"F2", KEY_F2}, {"F3", KEY_F3},  {"F4",  KEY_F4},
  {"F5", KEY_F5}, {"F6", KEY_F6}, {"F7", KEY_F7},  {"F8",  KEY_F8},
  {"F9", KEY_F9}, {"F10",KEY_F10},{"F11",KEY_F11}, {"F12", KEY_F12},
};

inline int           cursor     = 0;
inline unsigned long wait_until = 0;


inline uint8_t keyCode(String name) {
  name.trim();
  name.toUpperCase();
  for (auto& entry : KEYS) {
    if (name == entry.name) {
      return entry.code;
    }
  }
  if (name.length() == 1) {
    return (uint8_t)name[0];
  }
  return 0;
}


inline uint8_t mouseButton(String name) {
  name.trim();
  name.toUpperCase();
  if (name == "R" || name == "RIGHT") {
    return MOUSE_RIGHT;
  }
  if (name == "M" || name == "MIDDLE") {
    return MOUSE_MIDDLE;
  }
  return MOUSE_LEFT;
}


inline bool needsShift(uint8_t code) {
  if (code >= 'A' && code <= 'Z') {
    return true;
  }
  return strchr("!@#$%^&*()_+{}|:\"<>?~", (char)code) != nullptr;
}


inline void rewind() {
  cursor     = 0;
  wait_until = 0;
}


inline void releaseAll() {
  keyboard.releaseAll();
  mouse.release(MOUSE_LEFT);
  mouse.release(MOUSE_RIGHT);
  mouse.release(MOUSE_MIDDLE);
}


inline bool readLine(String& out) {
  const char* buffer = text;
  int length = (int)strlen(buffer);

  while (cursor < length && (buffer[cursor] == '\r' || buffer[cursor] == '\n')) {
    cursor++;
  }
  if (cursor >= length) {
    return false;
  }

  int line_start = cursor;
  while (cursor < length && buffer[cursor] != '\n' && buffer[cursor] != '\r') {
    cursor++;
  }
  out = String(buffer + line_start, cursor - line_start);
  out.trim();
  return true;
}


inline void exec(const String& line) {
  if (line.length() == 0 || line.startsWith("//")) {
    return;
  }

  int space_at = line.indexOf(' ');
  String command = space_at < 0 ? line : line.substring(0, space_at);
  String argument = space_at < 0 ? "" : line.substring(space_at + 1);
  command.toUpperCase();

  if (command == "WAIT") {
    wait_until = millis() + argument.toInt();

  } else if (command == "KEY") {
    uint8_t code = keyCode(argument);
    if (!code) {
      return;
    }
    bool with_shift = needsShift(code);
    if (with_shift) {
      keyboard.press(KEY_LEFT_SHIFT);
    }
    keyboard.press(code);
    delay(8);
    keyboard.release(code);
    if (with_shift) {
      keyboard.release(KEY_LEFT_SHIFT);
    }

  } else if (command == "KEYDOWN") {
    uint8_t code = keyCode(argument);
    if (code) {
      keyboard.press(code);
    }

  } else if (command == "KEYUP") {
    uint8_t code = keyCode(argument);
    if (code) {
      keyboard.release(code);
    }

  } else if (command == "STRING") {
    keyboard.print(argument);

  } else if (command == "STRINGLN") {
    keyboard.println(argument);

  } else if (command == "MOUSE") {
    int inner_space = argument.indexOf(' ');
    int x = argument.substring(0, inner_space < 0 ? argument.length() : inner_space).toInt();
    int y = inner_space < 0 ? 0 : argument.substring(inner_space + 1).toInt();
    mouse.move((int8_t)x, (int8_t)y);

  } else if (command == "WHEEL") {
    mouse.move(0, 0, (int8_t)argument.toInt());

  } else if (command == "CLICK") {
    mouse.click(mouseButton(argument));

  } else if (command == "PRESS") {
    mouse.press(mouseButton(argument));

  } else if (command == "RELEASE") {
    mouse.release(mouseButton(argument));

  } else if (command == "REPEAT") {
    cursor = 0;
  }
}

} // namespace detail


inline void init() {
  detail::keyboard.begin();
  detail::mouse.begin();
}


inline void start() {
  detail::rewind();
  running = true;
}


inline void stop() {
  running = false;
  detail::releaseAll();
  detail::rewind();
}


inline void toggle() {
  if (running) {
    stop();
  } else {
    start();
  }
}


inline void save(const String& body) {
  size_t length = body.length();
  if (length >= cfg::MAX_SCRIPT) {
    length = cfg::MAX_SCRIPT - 1;
  }
  memcpy(text, body.c_str(), length);
  text[length] = 0;
}


inline void loop() {
  if (!running) {
    return;
  }
  if (millis() < detail::wait_until) {
    return;
  }

  String line;
  if (!detail::readLine(line)) {
    stop();
    return;
  }
  detail::exec(line);
}

} // namespace script
