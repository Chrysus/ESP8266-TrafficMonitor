#ifndef KRY_CHANNEL_SCANNER_H
#define KRY_CHANNEL_SCANNER_H

class KRY_ChannelScanner {
 public:
  KRY_ChannelScanner();
  KRY_ChannelScanner(unsigned long scan_interval);

  void Start();
  void Update();

  unsigned short CurrentChannel();
  
 private:
  unsigned short cur_channel;
  unsigned long scan_interval;
  unsigned long prev_channel_switch_millis;

  void PrintChannelSwitch();
};

#endif //KRY_CHANNEL_SCANNER_H
