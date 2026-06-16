#ifndef CONFIG_H
#define CONFIG_H

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#if ORD_BOARD == c3_supermini
#include "pins_c3_supermini.h"
#else
#error "Unsupported board. Please define ORD_BOARD in platformio.ini"
#endif

constexpr const char* default_wifi_ssid = "myNetwork";
constexpr const char* default_wifi_password = "myPassword";

#ifndef ORD_NAME
#define ORD_NAME "OpenRideDash"
#endif

#ifndef ORD_SHORT_NAME
#define ORD_SHORT_NAME "ord"
#endif

#ifdef BUILDTAG
constexpr const char* default_hostname = ORD_SHORT_NAME "-" STR(BUILDTAG);
#else
constexpr const char* default_hostname = ORD_SHORT_NAME;
#endif

#define TFT_ROTATION 0

#if defined(ORD_VERSION) && defined(BUILDTAG) && defined(BUILD_TIMESTAMP)
#define WHOAMI ORD_NAME " v" ORD_VERSION STR(BUILDTAG) " b" BUILD_TIMESTAMP
#else
#define WHOAMI ORD_NAME
#endif

#endif  // CONFIG_H