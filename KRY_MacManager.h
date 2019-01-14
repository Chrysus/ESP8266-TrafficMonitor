#ifndef KRY_MAC_MANAGER_H
#define KRY_MAC_MANAGER_H

#define ACCESS_POINT_ARRAY_SIZE 64

// Forward Declarations
class KRY_Device;
struct ESPDatagramBeaconSSID;
struct KRY_AccessPoint;
struct KRY_Mac;

class KRY_MacManager {
 public:
  KRY_MacManager();

  void ProcessFrame(void *buf, int len);

  void PrintAccessPoints();
  void PrintConnections();
  void PrintStations();
  
 private:
  const struct KRY_AccessPoint * access_points[ACCESS_POINT_ARRAY_SIZE] = {0};
  const struct KRY_Mac * devices[ACCESS_POINT_ARRAY_SIZE] = {0};
  KRY_Device * device_array[ACCESS_POINT_ARRAY_SIZE] = {0};

  // methods
  void ProcessDatagram_012(void *buf, int len);
  void ProcessDatagram_060(void *buf, int len);
  void ProcessDatagram_128(void *buf, int len);

  void ProcessBeaconFrame(void *buf, int len);
  void ProcessDataFrame(void *buf, int len);
  void ProcessProbeRequestFrame(void *buf, int len);

  // Helper Methods
  struct KRY_AccessPoint * CreateAccessPoint(const struct KRY_Mac &mac_address, const struct ESPDatagramBeaconSSID *ssid);
  void AddAccessPoint(const struct KRY_AccessPoint *access_point);
  bool IsAccessPointKnown(const struct KRY_Mac &mac_address);
  bool IsAccessPointKnown(const struct KRY_AccessPoint &access_point);

  void AddDevice(const struct KRY_Mac *device_mac);
  void AddDevice(KRY_Device *device);
  bool IsDeviceKnown(const struct KRY_Mac &device);
  void AddAccessPointToDevice(const struct KRY_Mac *ap_mac, const struct KRY_Mac *device_mac);

  KRY_Device * GetDevice(const struct KRY_Mac *device_mac);
};

#endif // KRY_MAC_MANAGER_H
