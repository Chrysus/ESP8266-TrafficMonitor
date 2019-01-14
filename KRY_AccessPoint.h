#ifndef KRY_ACCESS_POINT
#define KRY_ACCESS_POINT

#include "KRY_Structures.h"

struct KRY_AccessPoint {
  KRY_Mac mac_address;
  unsigned char ssid_length;
  char ssid[32];                  //<-- ssid has a max length of 32 chars
};

// Helper Functions
namespace KRY {
int mac_cmp(const struct KRY_Mac &mac_1, const struct KRY_Mac &mac_2);
int ap_cmp(const struct KRY_AccessPoint &ap1, const struct KRY_AccessPoint &ap2);
void print_mac_address(const KRY_Mac &mac);
}

#endif //KRY_ACCESS_POINT
