#ifndef KRY_DEVICE_H
#define KRY_DEVICE_H

#include "KRY_Structures.h"

#define DEVICE_ACCESS_POINT_ARRAY_SIZE 16

/*
struct KRY_Device {
  struct KRY_Mac *mac_address;
  struct KRY_Mac * access_points[DEVICE_ACCESS_POINT_ARRAY_SIZE] = {0};
};
*/

// forward declarations
struct KRY_AccessPoint;

class KRY_Device {
 public:
  KRY_Device(const struct KRY_Mac &mac);
  KRY_Device(const struct KRY_Mac &mac, char *device_name, unsigned short name_len);

  void AddAccessPoint(const struct KRY_AccessPoint *ap);
  void AddBytesReceived(unsigned long bytes_received);
  void PrintDeviceInfo() const;

  void SetDeviceName(char *device_name, unsigned short len);

  struct KRY_Mac mac_address;
  
 private:
  //const struct KRY_Mac mac_address;
  const struct KRY_AccessPoint * access_points[DEVICE_ACCESS_POINT_ARRAY_SIZE] = {0};
  unsigned long bytes_received = 0;

  char *device_name = 0;
};

#endif //KRY_DEVICE_H
