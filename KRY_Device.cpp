#include "KRY_AccessPoint.h"
#include "KRY_Device.h"
#include <ESP8266WiFi.h>
#include <stdio.h>

KRY_Device::KRY_Device(const struct KRY_Mac &mac) {
  memcpy(&(this->mac_address), &mac, sizeof(struct KRY_Mac));
}

KRY_Device::KRY_Device(const struct KRY_Mac &mac, char *device_name, unsigned short name_len) {
  memcpy(&(this->mac_address), &mac, sizeof(struct KRY_Mac));
  this->SetDeviceName(device_name, name_len);
}

void KRY_Device::AddAccessPoint(const struct KRY_AccessPoint *ap) {
  for (int i = 0; i < DEVICE_ACCESS_POINT_ARRAY_SIZE; i++) {
    const struct KRY_AccessPoint *cur_ap = this->access_points[i];

    if (cur_ap == NULL) {
      this->access_points[i] = ap;
      break;
    }
    
    // Already there?
    if (KRY::ap_cmp(*cur_ap, *ap) == 0) {
      break;
    }
  }
}

void KRY_Device::AddBytesReceived(unsigned long bytes_received) {
  this->bytes_received += bytes_received;
}

void KRY_Device::AddBytesSent(unsigned long bytes_sent) {
  this->bytes_sent += bytes_sent;
}

void KRY_Device::PrintDeviceInfo() const {
  KRY::print_mac_address(this->mac_address);
  Serial.printf("\t[tx:(%12lu)\trx:(%12lu)]", this->bytes_sent, this->bytes_received);

  if (this->device_name != 0) {
    Serial.printf("\t(%s)", this->device_name);
  }
  
  for (int i = 0; i < DEVICE_ACCESS_POINT_ARRAY_SIZE; i++) {
    const struct KRY_AccessPoint *ap = this->access_points[i];
    if (ap != NULL) {
      Serial.printf("\n\r");
      Serial.printf("\t");
      if (ap->ssid_length > 0 && ap->ssid_length <= 32) {
        char ssid[33] = {0};
        memcpy(ssid, ap->ssid, ap->ssid_length);
        Serial.printf("%s", ssid);
      } else {
        KRY::print_mac_address(ap->mac_address);
      }
    }
  }
  Serial.printf("\n\r");
}

void KRY_Device::SetDeviceName(char *device_name, unsigned short len) {  
  if (this->device_name != 0) {
    delete(this->device_name);
  }
  
  this->device_name = new char[len + 1];
  strncpy(this->device_name, device_name, len);
  this->device_name[len] = '\0';
}
