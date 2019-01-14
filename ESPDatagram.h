// https://www.oreilly.com/library/view/80211-wireless-networks/0596100523/ch04.html

#ifndef ESP8266_DATAGRAM_H
#define ESP8266_DATAGRAM_H

// Frame Types

const unsigned kType_Management = 0b00;
const unsigned kType_Control    = 0b01;
const unsigned kType_Data       = 0b10;
const unsigned kType_Reserved   = 0b11;

// Management Frame Subtypes

const unsigned kSubtype_Mgmt_AssocReq = 0b0000;
const unsigned kSubtype_Mgmt_ProbeReq = 0b0100;
const unsigned kSubtype_Mgmt_Beacon   = 0b1000;
const unsigned kSubtype_Mgmt_Disassoc = 0b1010;

// Control Frame Subtypes


// Data Frame Subtypes

const unsigned kSubtype_Data                = 0b0000;
const unsigned kSubtype_Null                = 0b0010;
const unsigned kSubtype_Data_CF_Poll        = 0b0100;
const unsigned kSubtype_Data_CF_Ack         = 0b1000;
const unsigned kSubtype_Data_CF_Ack_CF_Poll = 0b1100;


/* The meaning of the three mac_address fields depends on values in the frame control
   field.

    ToDS    FromDS    Address 1    Address 2    Address 3
    0       0         DA           SA           BSSID
    1       0         BSSID        SA           DA
    0       1         DA           BSSID        SA
    1       1         RA           TA           DA

    DA = Destination Address
    SA = Source Address
    TA = Transmitter Address
    RA = Receiver Address
    BSSID = Basic Service Set Identifier

    Note - There is a distinction between the Source and the Transmitter - the Source
           is the creator of the datagram, whereas the Transmitter is simply sending
           the datagram; it may just be an intermediary between the Souce and Destination.

           Similarily, there is a distinction between the Destination and the Receiver -
           the Destination is the intended as the final address, whereas the Receiver may
           just be an intermediate step in the transmission.
*/

struct ESPDatagramFrameControl {
  unsigned protocol : 2;
  unsigned type : 2;
  unsigned subtype : 4;
  unsigned toDS : 1;
  unsigned fromDS : 1;
  unsigned moreFrag : 1;
  unsigned retry : 1;
  unsigned powerMgmt: 1;
  unsigned moreData : 1;
  unsigned protectedFrame : 1;
  unsigned order : 1;
};

struct ESPDatagramMACHeader {
  byte frame_control[2];
  byte duration[2];
  byte mac_address_1[6];
  byte mac_address_2[6];
  byte mac_address_3[6];
  byte seq_ctl[2];
};

struct ESPDatagramBeaconSSID {
  byte element_id;
  byte length;
  char *ssid;                 //<-- this is of variable length, up to 32 bytes
};

struct ESPDatagramBeaconFrameBody {
  byte timestamp[8];
  byte beacon_interval[2];
  byte capability_info[2];
  ESPDatagramBeaconSSID *ssid;
};

/*
void print_mac_address(const uint8_t *mac_address) {
  for (int i = 0; i < 5; i++) Serial.printf("%02x:", mac_address[i]);
  Serial.printf("%02x", mac_address[5]);
}
*/

/*
void print_ssid(const ESPDatagramBeaconSSID *ssid) {
  char *ssid_string = new char[ssid->length + 1];
  memcpy(ssid_string, ssid->ssid, ssid->length);
  ssid_string[ssid->length] = '\0';
  Serial.print(ssid_string);
};
*/

#endif // ESP8266_DATAGRAM_H
