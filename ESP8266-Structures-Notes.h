struct RxControl {
 signed rssi:8; // signal intensity of packet
 unsigned rate:4;
 unsigned is_group:1;
 unsigned:1;
 unsigned sig_mode:2; // 0:is not 11n packet; non-0:is 11n
packet;
 unsigned legacy_length:12; // if not 11n packet, shows length of
packet.
 unsigned damatch0:1;
 unsigned damatch1:1;
  unsigned bssidmatch0:1;
 unsigned bssidmatch1:1;
 unsigned MCS:7; // if is 11n packet, shows the
modulation
 // and code used (range from 0 to 76)
 unsigned CWB:1; // if is 11n packet, shows if is HT40 packet or
not
 unsigned HT_length:16;// if is 11n packet, shows length of
packet.
 unsigned Smoothing:1;
 unsigned Not_Sounding:1;
 unsigned:1;
 unsigned Aggregation:1;
 unsigned STBC:2;
 unsigned FEC_CODING:1; // if is 11n packet, shows if is LDPC
packet or not.
 unsigned SGI:1;
 unsigned rxend_state:8;
 unsigned ampdu_cnt:8;
 unsigned channel:4; //which channel this packet in.
 unsigned:12;
};
struct LenSeq{
 u16 len; // length of packet
 u16 seq; // serial number of packet, the high 12bits are serial
number,
 // low 14 bits are Fragment number (usually be 0)
 u8 addr3[6]; // the third address in packet
};
struct sniffer_buf{
 struct RxControl rx_ctrl;
 u8 buf[36]; // head of ieee80211 packet
 u16 cnt; // number count of packet
 struct LenSeq lenseq[1]; //length of packet
  };
struct sniffer_buf2{
 struct RxControl rx_ctrl;
 u8 buf[112]; //may be 240, please refer to the real source code
 u16 cnt;
 u16 len; //length of packet
};



/*

https://www.espressif.com/sites/default/files/documentation/esp8266-technical_reference_en.pdf


  Callback wifi_promiscuous_rx has two parameters (buf and len). len means the
length of buf, it can be: len = sizeof(struct sniffer_buf2), len = X * 10, len = sizeof(struct
RxControl):
Case of LEN == sizeof (struct sniffer_buf2)
• buf contains structure sniffer_buf2: it is the management packet, it has 112
Bytes data.
• sniffer_buf2.cnt is 1.
• sniffer_buf2.len is the length of packet.
Case of LEN == X * 10
• buf contains structure sniffer_buf: this structure is reliable, data packets
represented by it has been verified by CRC.
• sniffer_buf.cnt means the count of packets in buf. The value of len depends
on sniffer_buf.cnt.
- sniffer_buf.cnt==0, invalid buf; otherwise, len = 50 + cnt * 10
• sniffer_buf.buf contains the first 36 Bytes of IEEE80211 packet. Starting from
sniffer_buf.lenseq[0], each structure lenseq represent a length information of
packet. lenseq[0] represents the length of first packet. If there are two packets
where (sniffer_buf.cnt == 2), lenseq[1] represents the length of second
packet.
• If sniffer_buf.cnt > 1, it is a AMPDU packet, head of each MPDU packets are
similar, so we only provide the length of each packet (from head of MAC packet to
FCS)
• This structure contains: length of packet, MAC address of both sides of
communication, length of the head of packet.
Case of LEN == sizeof(struct RxControl)
• buf contains structure RxControl; but this structure is not reliable, we can not get
neither MAC address of both sides of communication nor length of the head of
packet.
• For AMPDU packet, we can not get the count of packets or the length of packet. 
This structure contains: length of packet, rssi and FEC_CODING.
• RSSI and FEC_CODING are used to guess if the packets are sent from same device.




For the case of LEN == sizeof(struct RxControl), the methods to calculate the length of packet are as below:
• If sig_mode == 0, the length of packet is the legacy_length.
• Otherwise, the length of packet is in struct sniffer_buf and sniffer_buf2, and it is more reliable

*/
