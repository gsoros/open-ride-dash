#ifndef CONFIG_H
#define CONFIG_H

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#if ORD_BOARD == c3_supermini
#include "pins_c3_supermini.h"
#else
#error "Unsupported board. Please define ORD_BOARD in platformio.ini"
#endif

constexpr const char* DEFAULT_WIFI_SSID = "myNetwork";
constexpr const char* DEFAULT_WIFI_PASSWORD = "myPassword";

// Fallback AP: if STA doesn't connect within this time, start a passwordless AP
constexpr unsigned long WIFI_STA_TIMEOUT_MS = 60000;
constexpr unsigned char WIFI_AP_CHANNEL = 1;
constexpr unsigned char WIFI_AP_MAX_CONNECTIONS = 2;

#ifndef ORD_NAME
// keep the format, build_info.py will parse this
#define ORD_NAME "OpenRideDash"
#endif

#ifndef ORD_SHORT_NAME
// keep the format, build_info.py will parse this
#define ORD_SHORT_NAME "ord"
#endif

#ifdef BUILDTAG
constexpr const char* DEFAULT_HOSTNAME = ORD_SHORT_NAME "-" STR(BUILDTAG);
#else
constexpr const char* DEFAULT_HOSTNAME = ORD_SHORT_NAME;
#endif

#ifndef TFT_ROTATION
#define TFT_ROTATION 3
#endif

// ST7789 column/row offsets used by Arduino_GFX/Arduino_TFT.
// These are panel/module specific and can be adjusted per module.
// Format: COL_OFFSET1, ROW_OFFSET1, COL_OFFSET2, ROW_OFFSET2
// Defaults are set to values known to work with the maintainer's module.
#ifndef ST7789_COL_OFFSET1
#define ST7789_COL_OFFSET1 0
#endif

#ifndef ST7789_ROW_OFFSET1
#define ST7789_ROW_OFFSET1 0
#endif

#ifndef ST7789_COL_OFFSET2
#define ST7789_COL_OFFSET2 0
#endif

#ifndef ST7789_ROW_OFFSET2
#define ST7789_ROW_OFFSET2 80
#endif

#endif  // CONFIG_H