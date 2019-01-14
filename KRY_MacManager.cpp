#include "KRY_MacManager.h"
#include <ESP8266WiFi.h>
#include "ESP8266-Structures.h"
#include "KRY_AccessPoint.h"
#include "KRY_Device.h"
#include "KRY_MyDevices.h"
#include <stdio.h>

KRY_MacManager::KRY_MacManager() {
  for (int i = 0; i < MY_DEVICE_ARRAY_LENGTH; i++) {
    this->device_array[i] = &my_devices[i];
  }
}

void KRY_MacManager::ProcessFrame(void *buf, int len) {
  switch(len) {
    case 12:
    {;
      this->ProcessDatagram_012(buf, len);
    }
    break;

    case 60:
    {
      this->ProcessDatagram_060(buf, len);
    }
    break;

    case 128:
    {
      this->ProcessDatagram_128(buf, len);
    }
    break;

    default:
    {
    }
    break;
  }
}

void KRY_MacManager::PrintAccessPoints() {
  static char ssid_string[33];
  
  Serial.printf("\n\r-------------------------------------------------------------------------------------\n\r");
  Serial.printf(" ACCESS POINTS");
  Serial.printf("\n\r-------------------------------------------------------------------------------------\n\r");

  const struct KRY_AccessPoint *cur_access_point = NULL;
  for(int i = 0; i < ACCESS_POINT_ARRAY_SIZE; i++) {
    cur_access_point = this->access_points[i];
    if (cur_access_point == NULL) {
      break;
    }

    uint8_t ssid_length = cur_access_point->ssid_length;
    memcpy(ssid_string, cur_access_point->ssid, ssid_length);
    ssid_string[ssid_length] = '\0';
  
    Serial.printf("%-32s:\t", ssid_string);
    KRY::print_mac_address(cur_access_point->mac_address);
    Serial.printf("\n\r");
  }
  Serial.printf("\n\r-------------------------------------------------------------------------------------\n\r");
}

void KRY_MacManager::PrintConnections() {
}

void KRY_MacManager::PrintStations() {
  Serial.printf("\n\r-------------------------------------------------------------------------------------\n\r");
  Serial.printf(" STATIONS");
  Serial.printf("\n\r-------------------------------------------------------------------------------------\n\r");

  const struct KRY_Mac *cur_station = NULL;
  const KRY_Device *cur_device = NULL;
  for(int i = 0; i < ACCESS_POINT_ARRAY_SIZE; i++) {
    cur_device = this->device_array[i];
    if (cur_device == NULL) {
      break;
    }

    cur_device->PrintDeviceInfo();
  }
  Serial.printf("\n\r-------------------------------------------------------------------------------------\n\r");
}


// Private Methods

// Process Datagram - 12
void KRY_MacManager::ProcessDatagram_012(void *buf, int len) {
}

// Process Datagram - 60
void KRY_MacManager::ProcessDatagram_060(void *buf, int len) {
  struct PromiscuousInfoSmall *packet_info = (struct PromiscuousInfoSmall *)buf;
  struct PacketHeader *packet_header = (struct PacketHeader *)&(packet_info->packet_header);
  struct FrameControl *frame_control = (FrameControl *)packet_header->frame_control;

  // MGMT
  if (frame_control->type == kType_Management) {
    if (frame_control->subtype == kSubtype_Mgmt_Beacon) {
      Serial.printf("SMALL BEACON FRAME?\n\r");
    }

    if (frame_control->subtype == kSubtype_Mgmt_ProbeReq) {
      Serial.printf("SMALL PROBE FRAME?\n\r");
    }
  }

  // DATA
  if (frame_control->type == kType_Data) {
    if (frame_control->subtype == kSubtype_Data) {
      this->ProcessDataFrame(buf, len);
    }
  }
}

// Process Datagram - 128
void KRY_MacManager::ProcessDatagram_128(void *buf, int len) {
  struct PromiscuousInfoSmall *packet_info = (struct PromiscuousInfoSmall *)buf;
  struct PacketHeader *packet_header = (struct PacketHeader *)&(packet_info->packet_header);
  struct FrameControl *frame_control = (FrameControl *)packet_header->frame_control;

  if (frame_control->type == kType_Management) {
    if (frame_control->subtype == kSubtype_Mgmt_Beacon) {
      this->ProcessBeaconFrame(buf, len);
    }

    if (frame_control->subtype == kSubtype_Mgmt_ProbeReq) {
      this->ProcessProbeRequestFrame(buf, len);
    }
  }
}


void KRY_MacManager::ProcessBeaconFrame(void *buf, int len) {
  struct PromiscuousInfoLarge *packet_info = (struct PromiscuousInfoLarge *)buf;
  void *frame_header_ptr = packet_info->buf;
  void *frame_body_ptr = frame_header_ptr + sizeof(struct BeaconFrameHeader);

  struct BeaconFrameHeader *frame_header = (struct BeaconFrameHeader *)frame_header_ptr;
  struct ESPDatagramBeaconFrameBody *frame_body = (struct ESPDatagramBeaconFrameBody *)frame_body_ptr;
  struct ESPDatagramBeaconSSID *ssid_ptr = &(frame_body->ssid);

  if (ssid_ptr->element_id != 0) {
    Serial.printf("ERROR - BAD ELEMENT ID");
  } else {
    struct KRY_AccessPoint *new_access_point = NULL;
    struct KRY_Mac *mac_address = (struct KRY_Mac *)(frame_header->mac_address_2);
    if (this->IsAccessPointKnown(*mac_address) != true) {
      new_access_point = this->CreateAccessPoint(*mac_address, ssid_ptr);
      this->AddAccessPoint(new_access_point);
    }
  }

  // HACK
  /*
  Serial.printf("[BEACON]\t");
  print_mac_address(frame_header->mac_address_1);
  Serial.printf("\t");
  print_mac_address(frame_header->mac_address_2);
  Serial.printf("\t");
  print_mac_address(frame_header->mac_address_3);
  Serial.printf("\n\r");
  */
}

void KRY_MacManager::ProcessDataFrame(void *buf, int len) {
  struct PromiscuousInfoSmall *packet_info = (struct PromiscuousInfoSmall *)buf;
  struct PacketHeader *packet_header = (struct PacketHeader *)&(packet_info->packet_header);
  struct FrameControl *frame_control = (FrameControl *)packet_header->frame_control;

  uint16_t data_len = packet_info->len;
  if (frame_control->subtype == kSubtype_Data) {
    struct KRY_Mac *mac_address = NULL;

    struct KRY_Mac *DA_MAC = NULL;
    struct KRY_Mac *SA_MAC = NULL;
    struct KRY_Mac *BSSID_MAC = NULL;

    if (frame_control->toDS == 0b00) {
      if (frame_control->fromDS == 0b00) {
        // 00
        DA_MAC = (struct KRY_Mac *)(packet_header->mac_address_1);
        SA_MAC = (struct KRY_Mac *)(packet_header->mac_address_2);
        BSSID_MAC = (struct KRY_Mac *)(packet_header->mac_address_3);
      } else {
        // 01
        DA_MAC = (struct KRY_Mac *)(packet_header->mac_address_1);
        SA_MAC = (struct KRY_Mac *)(packet_header->mac_address_3);
        BSSID_MAC = (struct KRY_Mac *)(packet_header->mac_address_2);
      }
    } else {
      if (frame_control->fromDS == 0b00) {
        // 10
        DA_MAC = (struct KRY_Mac *)(packet_header->mac_address_3);
        SA_MAC = (struct KRY_Mac *)(packet_header->mac_address_2);
        BSSID_MAC = (struct KRY_Mac *)(packet_header->mac_address_1);
      } else {
        // 11
        DA_MAC = (struct KRY_Mac *)(packet_header->mac_address_3);
        SA_MAC = (struct KRY_Mac *)(packet_header->mac_address_4);
      }
    }

    KRY_Device *device = this->GetDevice(DA_MAC);
    if (device != NULL) {
      device->AddBytesReceived(data_len);

      if (BSSID_MAC != NULL) {
        this->AddAccessPointToDevice(BSSID_MAC, DA_MAC);
      }
    }

    /*
    mac_address = (struct KRY_Mac *)(packet_header->mac_address_1);
    KRY_Device *device = this->GetDevice(mac_address);
    if (device == NULL) {
      //device = new KRY_Device(*mac_address);
      //this->AddDevice(device);

      mac_address = (struct KRY_Mac *)(packet_header->mac_address_2);
      device = this->GetDevice(mac_address);
      if (device == NULL) {
        mac_address = (struct KRY_Mac *)(packet_header->mac_address_3);
        device = this->GetDevice(mac_address);
        if (device == NULL) {
        } else {
          device->AddBytesReceived(data_len);
        }
      } else {
        device->AddBytesReceived(data_len);
      }
    } else {
      device->AddBytesReceived(data_len);
    }
  }
  */
  }
}

void KRY_MacManager::ProcessProbeRequestFrame(void *buf, int len) {
  struct PromiscuousInfoLarge *packet_info = (struct PromiscuousInfoLarge *)buf;
  void *frame_header_ptr = packet_info->buf;

  struct PacketHeader *frame_header = (struct PacketHeader *)frame_header_ptr;

  struct KRY_Mac *mac_address = (struct KRY_Mac *)(frame_header->mac_address_2);
  if (this->IsDeviceKnown(*mac_address) != true) {
    struct KRY_Mac *device = new KRY_Mac();
    memcpy(device, mac_address, 6);
    this->AddDevice(device);
  }

  // HACK
  this->AddAccessPointToDevice((struct KRY_Mac *)(frame_header->mac_address_1), mac_address);

  // HACK
  /*
  Serial.printf("[PROBE]\t");
  print_mac_address(frame_header->mac_address_1);
  Serial.printf("\t");
  print_mac_address(frame_header->mac_address_2);
  Serial.printf("\t");
  print_mac_address(frame_header->mac_address_3);
  Serial.printf("\n\r");
  */
}

struct KRY_AccessPoint * KRY_MacManager::CreateAccessPoint(const struct KRY_Mac &mac_address, const struct ESPDatagramBeaconSSID *ssid) {
  struct KRY_AccessPoint *new_access_point = new KRY_AccessPoint();
  memcpy(&(new_access_point->mac_address), &mac_address, 6);
  memcpy(new_access_point->ssid, ssid->ssid, ssid->length);
  new_access_point->ssid_length = ssid->length;

  return new_access_point;
}

void KRY_MacManager::AddAccessPoint(const struct KRY_AccessPoint *access_point) {
  if (IsAccessPointKnown(*access_point) == true) {
    return;
  }

  int next_index = 0;
  for (next_index = 0; next_index < ACCESS_POINT_ARRAY_SIZE; next_index++) {
    if (this->access_points[next_index] == NULL) {
      break;
    }
  }

  if (this->access_points[next_index] == NULL) {
    this->access_points[next_index] = access_point;
  }
}

bool KRY_MacManager::IsAccessPointKnown(const struct KRY_Mac &mac_address) {
  bool ret_value = false;

  const struct KRY_AccessPoint *cur_access_point = 0;
  for (unsigned int i = 0; i < ACCESS_POINT_ARRAY_SIZE; i++) {
    cur_access_point = this->access_points[i];
    if (cur_access_point == 0) {
      break;
    }

    if (KRY::mac_cmp(cur_access_point->mac_address, mac_address) == 0) {
      ret_value = true;
      break;
    }
  }

  return ret_value;
}

bool KRY_MacManager::IsAccessPointKnown(const struct KRY_AccessPoint &access_point) {
  bool ret_value = false;

  const struct KRY_AccessPoint *cur_access_point = NULL;
  for (unsigned int i = 0; i < ACCESS_POINT_ARRAY_SIZE; i++) {
    cur_access_point = this->access_points[i];
    if (cur_access_point == 0) {
      break;
    }

    if (KRY::ap_cmp(*cur_access_point, access_point) == 0) {
      ret_value = true;
      break;
    }
  }

  return ret_value;
 }
 
/*
 * Device Helpers
 */
 
void KRY_MacManager::AddDevice(const struct KRY_Mac *device) {
  if (IsDeviceKnown(*device) == true) {
    return;
  }

  int next_index = 0;
  for (next_index = 0; next_index < ACCESS_POINT_ARRAY_SIZE; next_index++) {
    if (this->device_array[next_index] == NULL) {
      break;
    }
  }
  
  if (this->device_array[next_index] == NULL) {
    KRY_Device *new_device = new KRY_Device(*device);
    this->device_array[next_index] = new_device;
  }
}

void KRY_MacManager::AddDevice(KRY_Device *device) {
  KRY_Device *found_device = this->GetDevice(&(device->mac_address));
  if (found_device != NULL) {
    return;
  }

  int next_index = 0;
  for (next_index = 0; next_index < ACCESS_POINT_ARRAY_SIZE; next_index++) {
    if (this->device_array[next_index] == NULL) {
      break;
    }
  }
  
  if (this->device_array[next_index] == NULL) {
    this->device_array[next_index] = device;
  }
}

bool KRY_MacManager::IsDeviceKnown(const struct KRY_Mac &device_mac) {
  bool ret_value = false;

  const struct KRY_Device *cur_device = NULL;
  for (unsigned int i = 0; i < ACCESS_POINT_ARRAY_SIZE; i++) {
    cur_device = this->device_array[i];
    if (cur_device == 0) {
      break;
    }

    if (KRY::mac_cmp(cur_device->mac_address, device_mac) == 0) {
      ret_value = true;
      break;
    }
  }

  return ret_value;
}

void KRY_MacManager::AddAccessPointToDevice(const struct KRY_Mac *ap_mac, const struct KRY_Mac *device_mac) {
  KRY_Device *cur_device = NULL;

  // Find the device
  for (unsigned int i = 0; i < ACCESS_POINT_ARRAY_SIZE; i++) {
    cur_device = this->device_array[i];
    if (cur_device == NULL) {
      break;
    }

    if (KRY::mac_cmp(cur_device->mac_address, *device_mac) == 0) {
      break;
    }
    cur_device = NULL;
  }

  if (cur_device != NULL) {
    const struct KRY_AccessPoint *cur_access_point = NULL;
    
    for (unsigned int i = 0; i < ACCESS_POINT_ARRAY_SIZE; i++) {
      cur_access_point = this->access_points[i];
      if (cur_access_point == 0) {
       break;
      }

      if (KRY::mac_cmp(cur_access_point->mac_address, *ap_mac) == 0) {
        break;
      }
      cur_access_point = NULL;
    }

    /*
    if (cur_access_point == NULL) {
      KRY_AccessPoint *new_access_point = new KRY_AccessPoint();
      memcpy(&(new_access_point->mac_address), ap_mac, 6);
      memset(new_access_point->ssid, 0, 32);
      new_access_point->ssid_length = 0;

      cur_access_point = new_access_point;
    }
    */
    if (cur_access_point != NULL) {
      cur_device->AddAccessPoint(cur_access_point);
    }
  }
}

KRY_Device * KRY_MacManager::GetDevice(const struct KRY_Mac *device_mac) {
  KRY_Device *cur_device = NULL;

  // Find the device
  for (unsigned int i = 0; i < ACCESS_POINT_ARRAY_SIZE; i++) {
    cur_device = this->device_array[i];
    if (cur_device == NULL) {
      break;
    }

    if (KRY::mac_cmp(cur_device->mac_address, *device_mac) == 0) {
      break;
    }
    cur_device = NULL;
  }

  return cur_device;
}
