#include <Arduino.h>
#include <WiFi.h>
#include "USB.h"
#include "config.h"
#include "script.h"
#include "touch.h"
#include "leds.h"
#include "web.h"


static void connectWiFi() {
  WiFi.mode(WIFI_STA);
  if (cfg::USE_STATIC_IP) {
    WiFi.config(cfg::IP_LOCAL, cfg::IP_GATEWAY, cfg::IP_SUBNET);
  }
  WiFi.begin(cfg::WIFI_SSID, cfg::WIFI_PASS);

  leds::wifi_connecting();
  Serial.print("WiFi");

  int polls_remaining = cfg::WIFI_TIMEOUT_S * 2;
  while (polls_remaining-- > 0 && WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Ready: http://");
    Serial.println(WiFi.localIP());
    leds::wifi_ready();
  } else {
    Serial.println("WiFi failed.");
    leds::wifi_failed();
  }
}


void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\nESP32-S3 HID Mimicker");

  USB.VID(cfg::HID_VID);
  USB.PID(cfg::HID_PID);
  USB.productName(cfg::HID_PRODUCT);
  USB.manufacturerName(cfg::HID_VENDOR);
  USB.serialNumber(cfg::HID_SERIAL);
  USB.begin();

  script::init();
  touch::init();
  connectWiFi();
  web::init();
}


void loop() {
  web::loop();
  script::loop();
  touch::loop();
  leds::loop();
}
