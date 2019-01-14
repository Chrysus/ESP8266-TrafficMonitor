#include <ESP8266WiFi.h>
#include <stdio.h>
#include "ESP8266-Structures.h"

void print_mac_address(const uint8_t *mac_address) {
  for (int i = 0; i < 5; i++) Serial.printf("%02x:", mac_address[i]);
  Serial.printf("%02x", mac_address[5]);
}
