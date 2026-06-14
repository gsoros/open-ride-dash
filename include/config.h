#ifndef CONFIG_H
#define CONFIG_H

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#if ORD_BOARD == c3_supermini
#include "pins_c3_supermini.h"
#else
#error "Unsupported board. Please define ORD_BOARD in platformio.ini"
#endif

const char* default_wifi_ssid = "myNetwork";
const char* default_wifi_password = "myPassword";

#ifdef BUILDTAG
const char* default_hostname = "ord-" STR(BUILDTAG);
#else
const char* default_hostname = "ord";
#endif

#define TFT_ROTATION 0

#endif  // CONFIG_H