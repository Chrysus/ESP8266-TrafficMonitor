/******************************************************************************* 
 *  Simple ESP8266 Sketch to demonstrate promiscuous mode and serial output
 *  
 *  This Sketch is designed for use with the ESP8266 microcontroller.  It sets
 *  the WiFi to promiscuous mode (meaning, it will simply "listen" to all WiFi
 *  traffic, including datagrams intended for other devices).
 *  
 *  In this version of the Sniffer, we will scan through the available WiFi
 *  channels, "listening" for 1 minute on each channel.
 *  
 *  This uses the wifi_set_promiscuous_rx_cb function to set a callback; this
 *  callback will be called whenever a WiFi datagram is detected.  In this
 *  Sketch, we will keep track of the number of datagrams detected, as well
 *  as the length of each datagram.  This tracking data will be periodically
 *  output over the Serial connection.
 *  
 *  As a bonus, the user can also request the data output by sending a
 *  newline (\n) to the ESP8266 via the Serial connection.
 *  
 *  Speeial thanks to Ray Burnette for his post on using the ESP8266 in 
 *  promiscuous mode:
 *  
 *  https://www.hackster.io/rayburne/esp8266-mini-sniff-f6b93a
 *  
 ******************************************************************************/


#include <ESP8266WiFi.h>
#include <stdio.h>
#include "ESP8266-Structures.h"
#include "KRY_AccessPoint.h"
#include "KRY_ChannelScanner.h"
#include "KRY_MacManager.h"

// For some reason on my ESP8266 board, the builtin LED is off when the pin
// is set to HIGH, and on when the pin is set to LOW.  So...

#define LED_ON       LOW
#define LED_OFF      HIGH

#define MODE_DISABLE 0
#define MODE_ENABLE  1

/*
 *  Global Variables
 */
byte gMyMAC[6];

const unsigned int kChannel = 0;  //<-- set this to 0 to scan (Channels range from 1 to 14)

// The following arrays will store the datagram counts for each channel; to 
// keep things simple, the index into the array for the channel will be the
// channel number.  However, since arrays are zero based, but the range of 
// channels start on 1, we will use index 0 to keep track of the total counts
// regardless of the channel.
unsigned long gCallbackCount[15] = {0};

// I happen to know that most datagrams will be 12, 60, or 128 bytes
unsigned long gDatagramCount_012[15] = {0};
unsigned long gDatagramCount_060[15] = {0};
unsigned long gDatagramCount_128[15] = {0};
unsigned long gDatagramCount_Other[15] = {0};

unsigned long gDatagramTypeMgmtCount[15] = {0};
unsigned long gDatagramTypeCtrlCount[15] = {0};
unsigned long gDatagramTypeDataCount[15] = {0};
unsigned long gDatagramTypeRsrvCount[15] = {0};

unsigned long gDatagramAssocCount[15] = {0};
unsigned long gDatagramProbeCount[15] = {0};
unsigned long gDatagramBeaconCount[15] = {0};
unsigned long gDatagramDisassocCount[15] = {0};

unsigned int gAccessPointCount = 0;
const unsigned int kAccessPointArraySize = 64;
struct KRY_AccessPoint *gAccessPoints[kAccessPointArraySize] = {0};

KRY_ChannelScanner *gChannelScanner = NULL;
KRY_MacManager *gMacManager = NULL;

/*
 *  Forward declarations
 */
void promiscuous_callback(uint8_t *buf, uint16_t len);
void print_report();
void print_ap_report();

void process_datagram_060(uint8_t *buf, uint16_t len);
void process_datagram_128(uint8_t *buf, uint16_t len);

void process_frame_control(const struct FrameControl &frame_control);
void process_packet_header(const struct PacketHeader &packet_header);

int mac_cmp(const byte &mac_1, const byte &mac_2);

bool IsAccessPointKnown(const struct KRY_AccessPoint &access_point);
void AddAccessPoint(struct KRY_AccessPoint *access_point);
struct KRY_AccessPoint * CreateAccessPoint(void *mac_address, struct ESPDatagramBeaconSSID *ssid);


void setup() {
  delay(500);
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_OFF);
  
  Serial.begin(115200);
  Serial.printf("\n\n\rSDK version:%s\n\r", system_get_sdk_version());
  Serial.println(F("ESP8266 - Traffic Monitor Sketch"));

  WiFi.macAddress(gMyMAC);
  Serial.print("\n\r -- MAC ADDRESS: ");
  print_mac_address(gMyMAC);
  Serial.print("\n\r");

  unsigned int current_channel = 1;
  if (kChannel) {
    current_channel = kChannel;
    print_channel_switch(current_channel);
  }

  // Promiscuous only works with STATION_MODE
  wifi_set_opmode(STATION_MODE);
  wifi_set_channel(current_channel);
  
  wifi_promiscuous_enable(MODE_DISABLE);
  wifi_set_promiscuous_rx_cb(promiscuous_callback);
  wifi_promiscuous_enable(MODE_ENABLE);

  if (kChannel == 0) {
    gChannelScanner = new KRY_ChannelScanner(1000 * 60);  //<-- 1000(ms) * 60(s) = 1(m)
    gChannelScanner->Start();
  }

  gMacManager = new KRY_MacManager(); 
}

/* 
 *  In the loop(), we blink the LED (on for 10 milliseconds) each time we
 *  detect (at least) 5 more datagrams.
 *  
 *  Note - a static variable within a function will retain its value between
 *  function calls.  Here we use prevCallbackCount to "remember" what the 
 *  callback count was the last time the LED was blinked.
 */
 
void loop() {
  static unsigned long prevBlinkCallbackCount = 0;
  static unsigned long prevPrintCallbackCount = 0;
  
  /*
   * Channel Scanning Logic
   */
   if (gChannelScanner) {
     gChannelScanner->Update();
   }
   
  /*
   * LED Blink Logic
   */
  long packetsDetectedSinceLastBlink = gCallbackCount[0] - prevBlinkCallbackCount;
  
  if (packetsDetectedSinceLastBlink >= 5) {
    digitalWrite(LED_BUILTIN, LED_ON);
    delay(10);  // delay in microseconds
    digitalWrite(LED_BUILTIN, LED_OFF);

    prevBlinkCallbackCount = gCallbackCount[0];
  }

  /*
   * Report Print Logic
   */
  bool bReportRequested = false;
  bool bFullReportRequested = false;
  bool bAccessPointReportRequested = false;

  // Did the user request a report?
  if ((Serial.available() > 0)) {
    char serial_input = Serial.read();
    if (serial_input == '\n') {
      bReportRequested = true;
    } else if (serial_input == 'F' || serial_input == 'f') {
      bFullReportRequested = true;
    } else if (serial_input == 'L' || serial_input == 'l') {
      bAccessPointReportRequested = true;
    }
  }
  
  long packetsDetectedSinceLastPrint = gCallbackCount[0] - prevPrintCallbackCount;
  
  if ( /*(packetsDetectedSinceLastPrint >= 500) || */ bReportRequested) {
    print_report();
    prevPrintCallbackCount = gCallbackCount[0];
  }

  // Print Full Report
  if (bFullReportRequested) {
    for (unsigned int i = 0; i < 15; i++) {
      print_channel_report(i);
    }
  }

  // Print Access Point Report
  if (bAccessPointReportRequested) {
    //print_ap_report();
    gMacManager->PrintAccessPoints();
    gMacManager->PrintStations();
  }
}

/*
 * Promiscuous Mode Callback
 */
 
void promiscuous_callback(uint8_t *buf, uint16_t len) {
  uint8_t cur_channel = kChannel;

  if (gChannelScanner) {
    cur_channel = gChannelScanner->CurrentChannel();
  }
  
  gCallbackCount[0]++;
  gCallbackCount[cur_channel]++;

  switch(len) {
    case 12:
    {
      gDatagramCount_012[0]++;
      gDatagramCount_012[cur_channel]++;
    }
    break;

    case 60:
    {
      gDatagramCount_060[0]++;
      gDatagramCount_060[cur_channel]++;

      //process_datagram_060(buf, len);
    }
    break;

    case 128:
    {
      gDatagramCount_128[0]++;
      gDatagramCount_128[cur_channel]++;

      //process_datagram_128(buf, len);
    }
    break;

    default:
    {
      gDatagramCount_Other[0]++;
      gDatagramCount_Other[cur_channel]++;
    }
    break;
  }

  gMacManager->ProcessFrame(buf, len);
}

/*
 * Datagram Processing Functions
 */
void process_datagram_060(uint8_t *buf, uint16_t len) {
  bool bShouldPrintDebugInfo = true;
  
  struct PromiscuousInfoSmall *packet_info = (struct PromiscuousInfoSmall *)buf;
  struct PacketHeader *packet_header = (struct PacketHeader *)&(packet_info->packet_header);

  // HACK
  FrameControl *frame_control = (FrameControl *)packet_header->frame_control;
  if (frame_control->type == kType_Data) {
    uint16_t data_len = packet_info->len;
    
    Serial.printf("DATA PACKET");
    if (frame_control->protectedFrame == 0b0) {
      Serial.printf(" [UNPROTECTED] ");
    } else {
      Serial.printf("               ");
    }
    Serial.printf("\t(%4d)", data_len);

    if (frame_control->subtype == kSubtype_Data) {
      Serial.printf(" - Data Only");
    }
    Serial.printf("\n\r");

    unsigned char *data = buf;
    for (int i = 0; i < len; i++) {
      if (data[i] >= 'A' && data[i] <= 'z') {
        Serial.printf("%c", data[i]);
      }
    }
    Serial.printf("\n\r");
  }
  // END HACK
}

void process_datagram_128(uint8_t *buf, uint16_t len) {
  bool bShouldPrintDebugInfo = true;
  
  struct PromiscuousInfoLarge *packet_info = (struct PromiscuousInfoLarge *)buf;
  struct PacketHeader *packet_header = (struct PacketHeader *)packet_info->buf;

  FrameControl *frame_control = (FrameControl *)packet_header->frame_control;

  // HACK
  if (frame_control->type == kType_Data) {
    uint16_t data_len = packet_info->len;
    
    Serial.printf("DATA PACKET");
    if (frame_control->protectedFrame == 0b0) {
      Serial.printf(" [UNPROTECTED] ");
    } else {
      Serial.printf("               ");
    }
    Serial.printf("\t(%4d)", data_len);

    if (frame_control->subtype == kSubtype_Data) {
      Serial.printf(" - Data Only");
    }
    Serial.printf("\n\r");
    
    /*
    if (data_len > 0) {
      char* packet_data = (char *)(packet_info->buf);
      char body[data_len + 1];
      strncpy(body, packet_data, data_len);
      Serial.printf("\t%s", body);
    }
    */
  }
  
  return;

/*
  bool bShouldPrintDebugInfo = false;

  if (frame_control->type == kType_Management) {
    gDatagramTypeMgmtCount[0]++;
    gDatagramTypeMgmtCount[gCurrentChannel]++;

    bool bShouldPrintMACAddresses = false;

    if (bShouldPrintDebugInfo) {
      Serial.print("MGMT");
      if (frame_control->subtype == kSubtype_Mgmt_AssocReq) {
        Serial.print("\tASSOC");
      } else if (frame_control->subtype == kSubtype_Mgmt_ProbeReq) {
        Serial.print("\tPROBE");
      } else if (frame_control->subtype == kSubtype_Mgmt_Beacon) {
        Serial.print("\tBEACON");
      } else if (frame_control->subtype == kSubtype_Mgmt_Disassoc) {
        Serial.print("\tDISSOC");
      } else {
        Serial.print("\tOTHER?");
      }
      bShouldPrintMACAddresses = true;
    }

    if (bShouldPrintMACAddresses) {
      Serial.print("\t");
      print_mac_address(packet_header->mac_address_1);
      Serial.print("\t");
      print_mac_address(packet_header->mac_address_2);
      Serial.print("\t");
      print_mac_address(packet_header->mac_address_3);
    }

    if (frame_control->subtype == kSubtype_Mgmt_Beacon) {
      if (bShouldPrintDebugInfo) {     
          Serial.print("\t");
      }
      
      void *frame_header_ptr = packet_info->buf;
      void *frame_body_ptr = frame_header_ptr + sizeof(struct BeaconFrameHeader);
        
      struct BeaconFrameHeader *frame_header = (struct BeaconFrameHeader *)frame_header_ptr;
      struct ESPDatagramBeaconFrameBody *frame_body = (struct ESPDatagramBeaconFrameBody *)frame_body_ptr;
      struct ESPDatagramBeaconSSID *ssid_ptr = &(frame_body->ssid);
        
      if (ssid_ptr->element_id != 0) {
        Serial.printf("ERROR - BAD ELEMENT ID");
      } else {
        memcpy(current_access_point.mac_address, frame_header->mac_address_2, 6);
        memcpy(current_access_point.ssid, ssid_ptr->ssid, ssid_ptr->length);
        current_access_point.ssid_length = ssid_ptr->length;

        struct KRY_AccessPoint *new_access_point = 0;

        if (IsAccessPointKnown(current_access_point) != true) {
          new_access_point = CreateAccessPoint(frame_header->mac_address_2, ssid_ptr);
          AddAccessPoint(new_access_point);

          print_ap_report();
        }
          
        if (new_access_point != 0 && bShouldPrintDebugInfo) {
          Serial.print("\t");
          print_mac_address(packet_header->mac_address_1);
          Serial.print("\t");
          print_mac_address(packet_header->mac_address_2);
          Serial.print("\t");
          print_mac_address(packet_header->mac_address_3);
          Serial.print("\t");
            
          uint8_t ssid_length = ssid_ptr->length;
          char *ssid_string = new char[ssid_length + 1]();
          memcpy(ssid_string, current_access_point.ssid, current_access_point.ssid_length);
          ssid_string[ssid_length] = '\0';
        
          Serial.printf("SSID - Length: %d\tValue: %s", new_access_point->ssid_length, ssid_string);

          delete(ssid_string);
        }
      }
    }
  } else if (frame_control->type == kType_Control) {
    gDatagramTypeCtrlCount[0]++;
    gDatagramTypeCtrlCount[gCurrentChannel]++;

    if (bShouldPrintDebugInfo) {
      Serial.print("CTRL");
      Serial.print("\n");
    }
  } else if (frame_control->type == kType_Data) {
    gDatagramTypeDataCount[0]++;
    gDatagramTypeDataCount[gCurrentChannel]++;

    if (bShouldPrintDebugInfo) {
      Serial.print("DATA");
      Serial.print("\n");
    }
  } else if (frame_control->type == kType_Reserved) {
    gDatagramTypeRsrvCount[0]++;
    gDatagramTypeRsrvCount[gCurrentChannel]++;

    if (bShouldPrintDebugInfo) {
      Serial.print("RSVD");
      Serial.print("\n");
    }
  } else {
      Serial.print("ERROR!");
      Serial.print("\n");
  }
*/
}

void process_frame_control(const struct FrameControl &frame_control) {
}

void process_packet_header(const struct PacketHeader &packet_header) {
  bool bShouldPrintDebugInfo = true;
  bool bShouldPrintMACAddresses = false;

  uint8_t cur_channel = kChannel;

  if (gChannelScanner) {
    cur_channel = gChannelScanner->CurrentChannel();
  }

  FrameControl *frame_control = (FrameControl *)packet_header.frame_control;
    
  if (frame_control->type == kType_Management) {
    gDatagramTypeMgmtCount[0]++;
    gDatagramTypeMgmtCount[cur_channel]++;

    if (bShouldPrintDebugInfo) {
      Serial.printf("MGMT\t");
      if (frame_control->subtype == kSubtype_Mgmt_AssocReq) {
        Serial.printf("ASSOC\t");
      } else if (frame_control->subtype == kSubtype_Mgmt_ProbeReq) {
        Serial.printf("PROBE\t");
      } else if (frame_control->subtype == kSubtype_Mgmt_Beacon) {
        Serial.printf("BEACON\t");
      } else if (frame_control->subtype == kSubtype_Mgmt_DisAssoc) {
        Serial.printf("DISSOC\t");
      }
      bShouldPrintMACAddresses = true;
    }
  } else if (frame_control->type == kType_Control) {
    gDatagramTypeCtrlCount[0]++;
    gDatagramTypeCtrlCount[cur_channel]++;

    if (bShouldPrintDebugInfo) {
      Serial.printf("CTRL\t");
      bShouldPrintMACAddresses = true;
    }
  } else if (frame_control->type == kType_Data) {
    gDatagramTypeDataCount[0]++;
    gDatagramTypeDataCount[cur_channel]++;

    if (bShouldPrintDebugInfo) {
      Serial.printf("DATA\t");
      bShouldPrintMACAddresses = true;
    }
  } else if (frame_control->type == kType_Reserved) {
    gDatagramTypeRsrvCount[0]++;
    gDatagramTypeRsrvCount[cur_channel]++;

    if (bShouldPrintDebugInfo) {
     Serial.printf("RSVD\t");
    }
  } else {
    Serial.printf("ERROR!\t");
  }

  if (bShouldPrintMACAddresses) {
    print_mac_address(packet_header.mac_address_1);
    Serial.printf("\t");
    print_mac_address(packet_header.mac_address_2);
    Serial.printf("\t");
    print_mac_address(packet_header.mac_address_3);
    Serial.printf("\t");
  }
}

/*
 * Print Functions
 */

 void print_channel_switch(unsigned int channel_index) {
   Serial.println("\n\r-------------------------------------------------------------------------------------");
   Serial.printf("SCANNING CHANNEL: %d\n\r", channel_index);
   Serial.println("-------------------------------------------------------------------------------------\n\n\r");
 }

 void print_report() {
   Serial.println("\n\r-------------------------------------------------------------------------------------");

   for (unsigned int channel_index = 1; channel_index < 15; channel_index++) {
     Serial.printf("CHANNEL %2d:\t%7lu\t\t%7lu\t%7lu\t%7lu\t%7lu\n", channel_index, gCallbackCount[channel_index], gDatagramTypeMgmtCount[channel_index], gDatagramTypeCtrlCount[channel_index], gDatagramTypeDataCount[channel_index], gDatagramTypeRsrvCount[channel_index]);
   }

   Serial.println("---------------------------");
   Serial.printf("TOTAL     :\t%7lu\t\t%7lu\t%7lu\t%7lu\t%7lu\n\r", gCallbackCount[0], gDatagramTypeMgmtCount[0], gDatagramTypeCtrlCount[0], gDatagramTypeDataCount[0], gDatagramTypeRsrvCount[0]);
   Serial.println("-------------------------------------------------------------------------------------");
 }

 void print_channel_report(unsigned int channel_index) {
   Serial.println("\n\r-------------------------------------------------------------------------------------");
   Serial.printf("CHANNEL: %d\n\r", channel_index);
   Serial.println("-------------------------------------------------------------------------------------");
   
   Serial.println("\n\r-------------------------------------------------------------------------------------\n\r");
   Serial.printf("Datagram    12:    %7lu\n\r", gDatagramCount_012[channel_index]);
   Serial.printf("Datagram    62:    %7lu\n\r", gDatagramCount_060[channel_index]);
   Serial.printf("Datagram   128:    %7lu\n\r", gDatagramCount_128[channel_index]);
   Serial.printf("Datagram Other:    %7lu\n\r", gDatagramCount_Other[channel_index]);
   Serial.println("---------------------------");
   Serial.printf("Type      Mgmt:    %7lu\n\r", gDatagramTypeMgmtCount[channel_index]);
   Serial.printf("Type      Cntl:    %7lu\n\r", gDatagramTypeCtrlCount[channel_index]);
   Serial.printf("Type      Data:    %7lu\n\r", gDatagramTypeDataCount[channel_index]);
   Serial.printf("Type      Rsrv:    %7lu\n\r", gDatagramTypeRsrvCount[channel_index]);
   Serial.println("---------------------------");
   Serial.printf("Type     Assoc:    %7lu\n\r", gDatagramAssocCount[channel_index]);
   Serial.printf("Type    Beacon:    %7lu\n\r", gDatagramBeaconCount[channel_index]);
   Serial.printf("Type     Probe:    %7lu\n\r", gDatagramProbeCount[channel_index]);
   Serial.printf("Type  DisAssoc:    %7lu\n\r", gDatagramDisassocCount[channel_index]);
   Serial.println("---------------------------");
   Serial.printf("Datagram Total:    %7lu\n\r", gCallbackCount[channel_index]);
   Serial.println("\n\r-------------------------------------------------------------------------------------\n\n\r");
 }

void print_ap_info(const struct KRY_AccessPoint *ap) {
  static char ssid_string[33];
  
  KRY::print_mac_address(ap->mac_address);
  Serial.print("\t");

  uint8_t ssid_length = ap->ssid_length;
  memcpy(ssid_string, ap->ssid, ssid_length);
  ssid_string[ssid_length] = '\0';
        
  Serial.printf("%s\n\r", ssid_string);
}

void print_ap_report() {
  Serial.println("\n\r-------------------------------------------------------------------------------------\n\r");
  Serial.println("--- ACCESS POINTS ---");
  struct KRY_AccessPoint *cur_access_point = 0;
  for (unsigned int i = 0; i < kAccessPointArraySize; i++) {
    cur_access_point = gAccessPoints[i];
    if (cur_access_point == 0) {
      break;
    }
    print_ap_info(cur_access_point);
  }
  Serial.println("\n\r--------------------------------------------------------------------------------------\n\r");
}

 /*
 * MAC Helper Functions
 */

int mac_cmp(const byte &mac_1, const byte &mac_2) {
  // this is incomplete
  return memcmp(&mac_1, &mac_2, 6);
}

 /*
 * Access Point Helper Functions
 */

/*
int ap_cmp(const struct KRY_AccessPoint &ap1, const struct KRY_AccessPoint &ap2) {
  // this is incomplete
  return memcmp(ap1.mac_address, ap2.mac_address, 6);
}
*/

bool IsAccessPointKnown(const struct KRY_AccessPoint &access_point) {
  bool ret_value = false;

  KRY_AccessPoint *cur_access_point = 0;
  for (unsigned int i = 0; i < kAccessPointArraySize; i++) {
    cur_access_point = gAccessPoints[i];
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

void AddAccessPoint(struct KRY_AccessPoint *access_point) {
  if (IsAccessPointKnown(*access_point) == true) {
    return;
  }

  if (gAccessPointCount >= kAccessPointArraySize) {
    return;
  }

  gAccessPoints[gAccessPointCount] = access_point;
  gAccessPointCount++;
}

struct KRY_AccessPoint * CreateAccessPoint(void *mac_address, struct ESPDatagramBeaconSSID *ssid) {
  struct KRY_AccessPoint *new_access_point = new KRY_AccessPoint();
  memcpy(&(new_access_point->mac_address), mac_address, 6);
  memcpy(&(new_access_point->ssid), ssid->ssid, ssid->length);
  new_access_point->ssid_length = ssid->length;

  return new_access_point;
}
