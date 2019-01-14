typedef struct {
    signed rssi:8;                /**< Received Signal Strength Indicator(RSSI) of packet. unit: dBm */
    unsigned rate:5;              /**< PHY rate encoding of the packet. Only valid for non HT(11bg) packet */
    unsigned :1;                  /**< reserve */
    unsigned sig_mode:2;          /**< 0: non HT(11bg) packet; 1: HT(11n) packet; 3: VHT(11ac) packet */
    unsigned :16;                 /**< reserve */
    unsigned mcs:7;               /**< Modulation Coding Scheme. If is HT(11n) packet, shows the modulation, range from 0 to 76(MSC0 ~ MCS76) */
    unsigned cwb:1;               /**< Channel Bandwidth of the packet. 0: 20MHz; 1: 40MHz */
    unsigned :16;                 /**< reserve */
    unsigned smoothing:1;         /**< reserve */
    unsigned not_sounding:1;      /**< reserve */
    unsigned :1;                  /**< reserve */
    unsigned aggregation:1;       /**< Aggregation. 0: MPDU packet; 1: AMPDU packet */
    unsigned stbc:2;              /**< Space Time Block Code(STBC). 0: non STBC packet; 1: STBC packet */
    unsigned fec_coding:1;        /**< Flag is set for 11n packets which are LDPC */
    unsigned sgi:1;               /**< Short Guide Interval(SGI). 0: Long GI; 1: Short GI */
    signed noise_floor:8;         /**< noise floor of Radio Frequency Module(RF). unit: 0.25dBm*/
    unsigned ampdu_cnt:8;         /**< ampdu cnt */
    unsigned channel:4;           /**< primary channel on which this packet is received */
    unsigned secondary_channel:4; /**< secondary channel on which this packet is received. 0: none; 1: above; 2: below */
    unsigned :8;                  /**< reserve */
    unsigned timestamp:32;        /**< timestamp. The local time when this packet is received. It is precise only if modem sleep or light sleep is not enabled. unit: microsecond */
    unsigned :32;                 /**< reserve */
    unsigned :31;                 /**< reserve */
    unsigned ant:1;               /**< antenna number from which this packet is received. 0: WiFi antenna 0; 1: WiFi antenna 1 */
    unsigned sig_len:12;          /**< length of packet including Frame Check Sequence(FCS) */
    unsigned :12;                 /**< reserve */
    unsigned rx_state:8;          /**< state of the packet. 0: no error; others: error numbers which are not public */
} wifi_pkt_rx_ctrl_t;
