#ifndef KRY_MY_DEVICES_H
#define KRY_MY_DEVICES_H

#include "KRY_AccessPoint.h"
#include "KRY_Device.h"

#define MY_DEVICE_ARRAY_LENGTH  2

/* 
   Add Your Devices Here

   This is just a simple way to pre-populate any device list
   with already known devices.  It allows you to give each
   device a name string, making it easier to identify
   each device in output strings.
 */

KRY_Mac PhoneMac = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
KRY_Device Phone = KRY_Device(PhoneMac, "Phone", 5);

KRY_Mac TabletMac = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
KRY_Device Tablet = KRY_Device(TabletMac, "Tablet", 6);

KRY_Device my_devices[] = {Phone, Tablet};

#endif // KRY_MY_DEVICES_H
