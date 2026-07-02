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
// keep the format, build_info.py will parse this
#define ORD_NAME "OpenRideDash"
#endif

#ifndef ORD_SHORT_NAME
// keep the format, build_info.py will parse this
#define ORD_SHORT_NAME "ord"
#endif

#ifdef BUILDTAG
constexpr const char* default_hostname = ORD_SHORT_NAME "-" STR(BUILDTAG);
#else
constexpr const char* default_hostname = ORD_SHORT_NAME;
#endif

#define TFT_ROTATION 3

// ST7789 column/row offsets used by Arduino_GFX/Arduino_TFT.
// These are panel/module specific and can be adjusted per module.
// Format: COL_OFFSET1, ROW_OFFSET1, COL_OFFSET2, ROW_OFFSET2
// Defaults are set to values known to work with the user's module.
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