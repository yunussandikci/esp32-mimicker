#include <Arduino.h>
#include "USB.h"
#include "config.h"
#include "script_service.h"
#include "leds_service.h"
#include "wifi_service.h"
#include "web_service.h"


void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\nESP32 HID Mimicker");

  USB.VID(cfg::HID_VID);
  USB.PID(cfg::HID_PID);
  USB.productName(cfg::HID_PRODUCT);
  USB.manufacturerName(cfg::HID_VENDOR);
  USB.serialNumber(cfg::HID_SERIAL);
  USB.begin();

  script::init();

  leds::wifi_connecting();
  wifi::init();
  if (wifi::is_ap()) {
    leds::wifi_ap_setup();
  } else {
    leds::wifi_ready();
  }

  web::init();
}


void loop() {
  wifi::loop();
  web::loop();
  script::loop();
  leds::loop();
}
