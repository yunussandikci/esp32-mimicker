#pragma once
#include <Arduino.h>
#include <USBHIDKeyboard.h>
#include <USBHIDMouse.h>
#include <Preferences.h>
#include "config.h"
#include "keymaps.h"

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

constexpr const char* NVS_NAMESPACE  = "script";
constexpr const char* NVS_KEY_TEXT   = "text";
constexpr const char* NVS_KEY_LAYOUT = "layout";

inline USBHIDKeyboard keyboard;
inline USBHIDMouse    mouse;
inline Preferences    prefs;

inline keymap::Layout active_layout = keymap::LAYOUT_US;

struct KeyName { const char* name; uint8_t code; };

inline const KeyName KEYS[] = {
  {"BACKSPACE", KEY_BACKSPACE}, {"CAPSLOCK", KEY_CAPS_LOCK},
  {"DELETE",    KEY_DELETE},    {"INSERT",   KEY_INSERT},
  {"PGDOWN",    KEY_PAGE_DOWN}, {"PGUP",     KEY_PAGE_UP},
  {"ENTER",     KEY_RETURN},    {"TAB",      KEY_TAB},
  {"SHIFT",     KEY_LEFT_SHIFT},{"CTRL",     KEY_LEFT_CTRL},
  {"ALT",       KEY_LEFT_ALT},  {"GUI",      KEY_LEFT_GUI},
  {"ESC",       KEY_ESC},       {"SPACE",    ' '},
  {"HOME",      KEY_HOME},      {"END",      KEY_END},
  {"UP",        KEY_UP_ARROW},  {"DOWN",     KEY_DOWN_ARROW},
  {"LEFT",      KEY_LEFT_ARROW},{"RIGHT",    KEY_RIGHT_ARROW},
  {"F1", KEY_F1}, {"F2", KEY_F2}, {"F3", KEY_F3},  {"F4",  KEY_F4},
  {"F5", KEY_F5}, {"F6", KEY_F6}, {"F7", KEY_F7},  {"F8",  KEY_F8},
  {"F9", KEY_F9}, {"F10",KEY_F10},{"F11",KEY_F11}, {"F12", KEY_F12},
};

inline int           cursor     = 0;
inline unsigned long wait_until = 0;


inline uint8_t namedKey(const String& arg) {
  String n = arg; n.trim(); n.toUpperCase();
  for (auto& e : KEYS) if (n == e.name) return e.code;
  return 0;
}


inline uint8_t mouseButton(const String& arg) {
  String n = arg; n.trim(); n.toUpperCase();
  if (n == "R") return MOUSE_RIGHT;
  if (n == "M") return MOUSE_MIDDLE;
  return MOUSE_LEFT;
}


// Resolves a Unicode codepoint to (Arduino keyboard arg, modifier bitmask).
// For US layout, the Arduino library's internal _asciimap handles the lookup,
// so we hand it raw ASCII and let it deal with shift. For other layouts we
// look up the position+modifiers in the keymap table.
struct CharMap { uint8_t k; uint8_t mods; bool ok; };

inline CharMap mapChar(uint32_t cp) {
  if (active_layout == keymap::LAYOUT_US) {
    if (cp < 128) return {(uint8_t)cp, 0, true};
    return {0, 0, false};
  }
  uint8_t hid, mods;
  if (keymap::find(active_layout, cp, &hid, &mods))
    return {(uint8_t)(0x88 + hid), mods, true};
  return {0, 0, false};
}


inline void pressChar(uint32_t cp) {
  CharMap m = mapChar(cp);
  if (!m.ok) return;
  if (m.mods & keymap::MOD_SHIFT) keyboard.press(KEY_LEFT_SHIFT);
  if (m.mods & keymap::MOD_ALTGR) keyboard.press(KEY_RIGHT_ALT);
  keyboard.press(m.k);
}


inline void releaseChar(uint32_t cp) {
  CharMap m = mapChar(cp);
  if (!m.ok) return;
  keyboard.release(m.k);
  if (m.mods & keymap::MOD_ALTGR) keyboard.release(KEY_RIGHT_ALT);
  if (m.mods & keymap::MOD_SHIFT) keyboard.release(KEY_LEFT_SHIFT);
}


inline void typeChar(uint32_t cp) {
  pressChar(cp);
  delay(8);
  releaseChar(cp);
}


inline uint32_t firstCodepoint(const String& s) {
  uint32_t cp;
  return keymap::utf8_decode(s.c_str(), s.length(), &cp) > 0 ? cp : 0;
}


inline void typeUtf8(const String& s) {
  const char* p = s.c_str();
  size_t remaining = s.length();
  while (remaining > 0) {
    uint32_t cp;
    size_t consumed = keymap::utf8_decode(p, remaining, &cp);
    if (consumed == 0) break;
    typeChar(cp);
    p += consumed;
    remaining -= consumed;
  }
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

  while (cursor < length && (buffer[cursor] == '\r' || buffer[cursor] == '\n')) cursor++;
  if (cursor >= length) return false;

  int line_start = cursor;
  while (cursor < length && buffer[cursor] != '\n' && buffer[cursor] != '\r') cursor++;
  out = String(buffer + line_start, cursor - line_start);
  out.trim();
  return true;
}


inline void exec(const String& line) {
  if (line.length() == 0 || line.startsWith("//")) return;

  int space_at = line.indexOf(' ');
  String command = space_at < 0 ? line : line.substring(0, space_at);
  String argument = space_at < 0 ? "" : line.substring(space_at + 1);
  command.toUpperCase();

  if (command == "WAIT") {
    wait_until = millis() + argument.toInt();

  } else if (command == "KEY") {
    uint8_t code = namedKey(argument);
    if (code) {
      keyboard.press(code);
      delay(8);
      keyboard.release(code);
    } else {
      typeChar(firstCodepoint(argument));
    }

  } else if (command == "KEYDOWN") {
    uint8_t code = namedKey(argument);
    if (code) keyboard.press(code);
    else      pressChar(firstCodepoint(argument));

  } else if (command == "KEYUP") {
    uint8_t code = namedKey(argument);
    if (code) keyboard.release(code);
    else      releaseChar(firstCodepoint(argument));

  } else if (command == "STRING") {
    typeUtf8(argument);

  } else if (command == "STRINGLN") {
    typeUtf8(argument);
    typeChar('\n');

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
  detail::prefs.begin(detail::NVS_NAMESPACE, false);

  String saved = detail::prefs.getString(detail::NVS_KEY_TEXT, "");
  if (saved.length() > 0) {
    size_t length = saved.length();
    if (length >= cfg::MAX_SCRIPT) length = cfg::MAX_SCRIPT - 1;
    memcpy(text, saved.c_str(), length);
    text[length] = 0;
  }

  uint8_t saved_layout = detail::prefs.getUChar(detail::NVS_KEY_LAYOUT, 0);
  if (saved_layout < keymap::LAYOUT_COUNT) {
    detail::active_layout = (keymap::Layout)saved_layout;
  }
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


inline void save(const String& body) {
  size_t length = body.length();
  if (length >= cfg::MAX_SCRIPT) length = cfg::MAX_SCRIPT - 1;
  memcpy(text, body.c_str(), length);
  text[length] = 0;
  detail::prefs.putString(detail::NVS_KEY_TEXT, text);
}


inline keymap::Layout getLayout() { return detail::active_layout; }


inline void setLayout(keymap::Layout l) {
  detail::active_layout = l;
  detail::prefs.putUChar(detail::NVS_KEY_LAYOUT, (uint8_t)l);
}


inline void loop() {
  if (!running) return;
  if (millis() < detail::wait_until) return;

  String line;
  if (!detail::readLine(line)) {
    stop();
    return;
  }
  detail::exec(line);
}

} // namespace script
