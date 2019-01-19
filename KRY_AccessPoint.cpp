#include "KRY_AccessPoint.h"
#include <ESP8266WiFi.h>

namespace KRY {
  
int mac_cmp(const struct KRY_Mac &mac_1, const struct KRY_Mac &mac_2) {
  return memcmp(&mac_1, &mac_2, 6);
}

int ap_cmp(const struct KRY_AccessPoint &ap1, const struct KRY_AccessPoint &ap2) {
  unsigned int mac_cmp_value = 0;
  unsigned int ssid_cmp_value = 0;
  unsigned int ssid_len_cmp_value = 0;

  if (ap1.ssid_length != ap2.ssid_length) {
    return -1;
  }
  
  const void *mac_address_1 = &(ap1.mac_address);
  const void *mac_address_2 = &(ap2.mac_address);
  mac_cmp_value = memcmp(mac_address_1, mac_address_2, 6);
  ssid_cmp_value = strncmp(ap1.ssid, ap2.ssid, ap1.ssid_length);

  return mac_cmp_value + ssid_cmp_value;
}

void print_mac_address(const KRY_Mac &mac) {
  for (int i = 0; i < 5; i++) Serial.printf("%02x:", mac.address[i]);
  Serial.printf("%02x", mac.address[5]);
}
}
