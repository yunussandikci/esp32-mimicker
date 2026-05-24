#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <DNSServer.h>
#include "config.h"

namespace wifi {

namespace detail {

constexpr const char* NVS_NAMESPACE = "wifi";
constexpr const char* NVS_KEY_SSID  = "ssid";
constexpr const char* NVS_KEY_PASS  = "pass";

inline Preferences  prefs;
inline DNSServer    dns_server;
inline bool         in_ap_mode         = false;
inline unsigned long disconnected_at_ms = 0;


inline String storedSsid() {
  return prefs.getString(NVS_KEY_SSID, "");
}


inline String storedPass() {
  return prefs.getString(NVS_KEY_PASS, "");
}


inline void startAp() {
  in_ap_mode = true;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(cfg::AP_SSID);
  IPAddress ap_ip = WiFi.softAPIP();
  dns_server.start(53, "*", ap_ip);
  Serial.print("AP up. Connect to '");
  Serial.print(cfg::AP_SSID);
  Serial.print("' then open http://");
  Serial.println(ap_ip);
}


inline bool joinSta(const String& ssid, const String& pass) {
  in_ap_mode = false;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.print("STA ");
  Serial.print(ssid);

  int polls_remaining = cfg::WIFI_TIMEOUT_S * 2;
  while (polls_remaining-- > 0 && WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();
  return WiFi.status() == WL_CONNECTED;
}

} // namespace detail


inline bool is_ap() {
  return detail::in_ap_mode;
}


inline bool is_connected() {
  return !detail::in_ap_mode && WiFi.status() == WL_CONNECTED;
}


inline IPAddress ip() {
  return detail::in_ap_mode ? WiFi.softAPIP() : WiFi.localIP();
}


inline void save_credentials(const String& ssid, const String& pass) {
  detail::prefs.putString(detail::NVS_KEY_SSID, ssid);
  detail::prefs.putString(detail::NVS_KEY_PASS, pass);
}


inline void clear_credentials() {
  detail::prefs.remove(detail::NVS_KEY_SSID);
  detail::prefs.remove(detail::NVS_KEY_PASS);
}


inline void init() {
  detail::prefs.begin(detail::NVS_NAMESPACE, false);

  String ssid = detail::storedSsid();
  if (ssid.length() == 0) {
    Serial.println("No saved WiFi creds — starting AP");
    detail::startAp();
    return;
  }

  bool connected = detail::joinSta(ssid, detail::storedPass());
  if (!connected) {
    Serial.println("STA failed — wiping creds, starting AP");
    clear_credentials();
    detail::startAp();
    return;
  }

  Serial.print("Connected. http://");
  Serial.println(WiFi.localIP());
}


inline void loop() {
  if (detail::in_ap_mode) {
    detail::dns_server.processNextRequest();
    return;
  }

  unsigned long now_ms = millis();
  if (WiFi.status() == WL_CONNECTED) {
    detail::disconnected_at_ms = 0;
    return;
  }
  if (detail::disconnected_at_ms == 0) {
    detail::disconnected_at_ms = now_ms;
    return;
  }
  if (now_ms - detail::disconnected_at_ms > cfg::WIFI_DROP_WIPE_MS) {
    Serial.println("STA dropped too long — wiping & rebooting");
    clear_credentials();
    delay(200);
    ESP.restart();
  }
}

} // namespace wifi
