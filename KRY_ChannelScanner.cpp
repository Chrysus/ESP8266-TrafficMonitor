#include "KRY_ChannelScanner.h"
#include <ESP8266WiFi.h>
#include <stdio.h>

KRY_ChannelScanner::KRY_ChannelScanner() {
  this->scan_interval = 10000;
  this->prev_channel_switch_millis = 0;
  this->cur_channel = 1;
}

KRY_ChannelScanner::KRY_ChannelScanner(unsigned long scan_interval) {
  this->scan_interval = scan_interval;
  this->prev_channel_switch_millis = 0;
  this->cur_channel = 1;
}

void KRY_ChannelScanner::Start() {
  wifi_set_channel(this->cur_channel);
  this->prev_channel_switch_millis = millis();
  this->PrintChannelSwitch();
}

void KRY_ChannelScanner::Update() {
  unsigned long current_time = millis();
  unsigned long delta_time = current_time - this->prev_channel_switch_millis;

  if (delta_time > this->scan_interval) {
    this->cur_channel++;
    if (this->cur_channel > 14) {
      this->cur_channel = 1;
    }
    
    wifi_set_channel(this->cur_channel);
    this->prev_channel_switch_millis = current_time;
    this->PrintChannelSwitch();
  }
};

unsigned short KRY_ChannelScanner::CurrentChannel() {
  return this->cur_channel;
}


void KRY_ChannelScanner::PrintChannelSwitch() {
  Serial.printf("\n\r-------------------------------------------------------------------------------------\n\r");
  Serial.printf("SCANNING CHANNEL: %d\n\r", this->cur_channel);
  Serial.printf("-------------------------------------------------------------------------------------\n\n\r");
}
