#include "KRY_MacManager.h"
#include <ESP8266WiFi.h>
#include "ESP8266-Structures.h"
#include "KRY_AccessPoint.h"
#include "KRY_Device.h"
#include "KRY_MyDevices.h"
#include <stdio.h>

KRY_MacManager::KRY_MacManager() {
  for (int i = 0; i < MY_DEVICE_ARRAY_LENGTH; i++) {
    this->AddDevice(&my_devices[i]);
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

  const struct KRY_AccessPoint *cur_access_point = this->access_points_list;
  
  while (cur_access_point != 0) {
    uint8_t ssid_length = cur_access_point->ssid_length;
    memcpy(ssid_string, cur_access_point->ssid, ssid_length);
    ssid_string[ssid_length] = '\0';
  
    Serial.printf("%-32s:\t", ssid_string);
    KRY::print_mac_address(cur_access_point->mac_address);
    Serial.printf("\n\r");

    cur_access_point = cur_access_point->next;
  }
  
  Serial.printf("\n\r-------------------------------------------------------------------------------------\n\r");
}

void KRY_MacManager::PrintConnections() {
}

void KRY_MacManager::PrintStations() {
  Serial.printf("\n\r-------------------------------------------------------------------------------------\n\r");
  Serial.printf(" STATIONS");
  Serial.printf("\n\r-------------------------------------------------------------------------------------\n\r");

  const KRY_Device *cur_device = this->devices_list;

  while (cur_device != 0) {
    cur_device->PrintDeviceInfo();
    cur_device = cur_device->next;
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

/*
 * BEACON
 */
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
    struct KRY_Mac *mac_address = (struct KRY_Mac *)(frame_header->mac_address_2);
    if (this->IsAccessPointKnown(*mac_address) != true) {
      // Check SSID first...
      struct KRY_AccessPoint *access_point = this->GetAccessPointWithSSID(ssid_ptr->ssid, ssid_ptr->length);
      if (access_point != NULL) {
        // Access point already recorded, but with different MAC
        //memcpy(&(new_access_point->mac_address), &mac_address, 6);
      } else {
        this->AddAccessPoint(*mac_address, ssid_ptr->ssid, ssid_ptr->length);
      }
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

/*
 * DATA
 */
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

    KRY_Device *tx_device = this->GetDevice(SA_MAC);
    if (tx_device != NULL) {
      tx_device->AddBytesSent(data_len);

      if (BSSID_MAC != NULL) {
        this->AddAccessPointToDevice(BSSID_MAC, SA_MAC);
      }
    }
  }
}

/*
 * PROBE
 */
void KRY_MacManager::ProcessProbeRequestFrame(void *buf, int len) {
  static const struct KRY_Mac NA_MAC = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  static const char *ssid_unknown = "UNKNOWN";
  static const unsigned short ssid_len = 7;
  
  struct PromiscuousInfoLarge *packet_info = (struct PromiscuousInfoLarge *)buf;
  void *frame_header_ptr = packet_info->buf;
  void *frame_body_ptr = frame_header_ptr + sizeof(struct BeaconFrameHeader);

  struct PacketHeader *frame_header = (struct PacketHeader *)frame_header_ptr;
  struct ESPDatagramBeaconSSID *ssid_ptr = (struct ESPDatagramBeaconSSID *)frame_body_ptr;

  struct KRY_Mac *DA_MAC = NULL;
  struct KRY_Mac *SA_MAC = NULL;
  struct KRY_Mac *BSSID_MAC = NULL;

  DA_MAC = (struct KRY_Mac *)(frame_header->mac_address_1);
  SA_MAC = (struct KRY_Mac *)(frame_header->mac_address_2);
  BSSID_MAC = (struct KRY_Mac *)(frame_header->mac_address_3);

  if (this->IsDeviceKnown(*SA_MAC) != true) {
    this->AddDeviceWithMac(*SA_MAC);

    /*
    // HACK
    if(ssid_ptr->length > 0 && ssid_ptr->length <= 32) {
      char ssid_str[33];
      strncpy(ssid_str, ssid_ptr->ssid, ssid_ptr->length);
      ssid_str[ssid_ptr->length] = '\0';
      KRY::print_mac_address(*SA_MAC);
      Serial.printf("\t%s\n\r", ssid_str);
    }
    */
  }

  /*
  // directed probe
  if(ssid_ptr->length > 0 && ssid_ptr->length <= 32) {
    struct KRY_AccessPoint *access_point = this->GetAccessPointWithSSID(ssid_ptr->ssid, ssid_ptr->length);
    if (access_point == NULL) {
      access_point = this->CreateAccessPoint(*BSSID_MAC, ssid_ptr->ssid, ssid_ptr->length);
      this->AddAccessPoint(access_point);
    }
    
    KRY_Device *device = this->GetDevice(SA_MAC);
    if (device != NULL) {
      device->AddAccessPoint(access_point);
    }
  }

  if (KRY::mac_cmp(*BSSID_MAC, NA_MAC) == 0) {
    // This is a broadcast probe
  } else {
    struct KRY_AccessPoint *new_access_point = NULL;
    if (this->IsAccessPointKnown(*BSSID_MAC) != true) {
      new_access_point = this->CreateAccessPoint(*BSSID_MAC, ssid_unknown, ssid_len);
      this->AddAccessPoint(new_access_point);
    }
    
    this->AddAccessPointToDevice(BSSID_MAC, SA_MAC);
  }
  */

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

void KRY_MacManager::AddAccessPoint(const struct KRY_Mac &mac_address, const char *ssid_string, unsigned short ssid_len) {
  if (ssid_len > 32) {
    Serial.printf("Error - bad SSID length: %d\n\r", ssid_len);
    return;  
  }
  
  struct KRY_AccessPoint *new_access_point = new KRY_AccessPoint();
  if (new_access_point != NULL) {
    memcpy(&(new_access_point->mac_address), &mac_address, 6);
    memcpy(new_access_point->ssid, ssid_string, ssid_len);
    new_access_point->ssid_length = ssid_len;

    if (this->IsAccessPointKnown(*new_access_point) == true) {
      delete(new_access_point);
      return;
    }

    if (this->access_points_list == 0) {
      this->access_points_list = new_access_point;
      this->access_points_tail = new_access_point;
    } else {
      this->access_points_tail->next = new_access_point;
      this->access_points_tail = new_access_point;
    }
  }
}

bool KRY_MacManager::IsAccessPointKnown(const struct KRY_Mac &mac_address) {
  bool ret_value = false;

  const struct KRY_AccessPoint *cur_access_point = this->access_points_list;
  while (cur_access_point != 0) {
    if (KRY::mac_cmp(cur_access_point->mac_address, mac_address) == 0) {
      ret_value = true;
      break;
    }
    cur_access_point = cur_access_point->next;
  }
  return ret_value;
}

bool KRY_MacManager::IsAccessPointKnown(const struct KRY_AccessPoint &access_point) {
  bool ret_value = false;

  const struct KRY_AccessPoint *cur_access_point = this->access_points_list;
  while (cur_access_point != NULL) {
    if (KRY::ap_cmp(*cur_access_point, access_point) == 0) {
      ret_value = true;
      break;
    }
    cur_access_point = cur_access_point->next;
  }
  return ret_value;
}
 
/*
 * Device Helpers
 */
 
void KRY_MacManager::AddDeviceWithMac(const struct KRY_Mac &device_mac) {  
  if (IsDeviceKnown(device_mac) == true) {
    return;
  }
  
  KRY_Device *new_device = new KRY_Device(device_mac);
  if (new_device == NULL) {
    Serial.printf("Error - failed to alloc new device\n\r");
    return;
  }
  
  if (this->devices_list == 0) {
    this->devices_list = new_device;
    this->devices_tail = new_device;
  } else {
    this->devices_tail->next = new_device;
    this->devices_tail = new_device;
  }
}

void KRY_MacManager::AddDevice(KRY_Device *device) {
  if (device == NULL) {
    Serial.println("Error - cannot add NULL device\n\r");
    return;
  }
  
  KRY_Device *found_device = this->GetDevice(&(device->mac_address));
  if (found_device != NULL) {
    return;
  }

  if (this->devices_list == 0) {
    this->devices_list = device;
    this->devices_tail = device;
  } else {
    this->devices_tail->next = device;
    this->devices_tail = device;
  }
}

bool KRY_MacManager::IsDeviceKnown(const struct KRY_Mac &device_mac) {
  bool ret_value = false;

  KRY_Device *cur_device = this->devices_list;
  while (cur_device != 0) {
    if (KRY::mac_cmp(cur_device->mac_address, device_mac) == 0) {
      ret_value = true;
      break;
    }
    cur_device = cur_device->next;
  }

  return ret_value;
}

void KRY_MacManager::AddAccessPointToDevice(const struct KRY_Mac *ap_mac, const struct KRY_Mac *device_mac) {
  KRY_Device *cur_device = this->GetDevice(device_mac);
  const struct KRY_AccessPoint *cur_access_point = this->GetAccessPoint(ap_mac);

  if (cur_device != NULL && cur_access_point != NULL) {
    cur_device->AddAccessPoint(cur_access_point);
  }
}

struct KRY_AccessPoint * KRY_MacManager::GetAccessPoint(const struct KRY_Mac *ap_mac) {
  struct KRY_AccessPoint *cur_access_point = this->access_points_list;

  if (ap_mac == NULL) {
    return NULL;
  }

  while(cur_access_point != 0) {
    if (KRY::mac_cmp(cur_access_point->mac_address, *ap_mac) == 0) {
      break;
    }
    cur_access_point = cur_access_point->next;
  }

  return cur_access_point;
}

struct KRY_AccessPoint * KRY_MacManager::GetAccessPointWithSSID(const char* ssid, unsigned short ssid_len) {
  struct KRY_AccessPoint *cur_access_point = this->access_points_list;

  if (ssid == NULL || ssid_len > 32) {
    return NULL;
  }

  while(cur_access_point != 0) {
    if (ssid_len == cur_access_point->ssid_length) {
      if (strncmp(cur_access_point->ssid, ssid, cur_access_point->ssid_length) == 0) {
        break;
      }
    }
    cur_access_point = cur_access_point->next;
  }

  return cur_access_point;
}

KRY_Device * KRY_MacManager::GetDevice(const struct KRY_Mac *device_mac) {
  KRY_Device *cur_device = this->devices_list;

  if (device_mac == NULL) {
    return NULL;
  }

  while(cur_device != 0) {
    if (KRY::mac_cmp(cur_device->mac_address, *device_mac) == 0) {
      break;
    }
    cur_device = cur_device->next;
  }

  return cur_device;
}
