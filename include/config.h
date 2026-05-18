#ifndef CONFIG_H
#define CONFIG_H

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

const char* default_wifi_ssid = "myNetwork";
const char* default_wifi_password = "myPassword";
#ifdef BUILDTAG
const char* default_hostname = "ord-" STR(BUILDTAG);
#else
const char* default_hostname = "ord";
#endif

#endif  // CONFIG_H