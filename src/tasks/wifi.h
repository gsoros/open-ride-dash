#ifndef WIFI_H
#define WIFI_H

/*
 *  TODOs in order of priority:
 *
 *  Find out why the system sometimes becomes bogged down (barely responsive)
 *  near the edge of WiFi range. Not a huge problem as WiFi is off during
 *  normal operation, but it's annoying while debugging.
 *
 *  Serial output is extremely laggy during OTA updates. OTA being prioritized
 *  over serial output is good, but let's research how the priority can be
 *  manipulated. It has to do with task priority and task loop frequency.
 *
 *  Manage dependent tasks (OTA, WifiSerial) based on WiFi mode instead of
 *  rebooting on mode change? Likely too complex and error-prone for little
 *  benefit. WiFi is used only for OTA updates and WifiSerial debugging, so
 *  restarting the system is not a big deal.
 *
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <cstring>
#include "task.h"
#include "has_preferences.h"
#include "config.h"
#include "api.h"
#include "tasks/wifiserial.h"
#include "tasks/display.h"

extern WifiSerial wifiSerial;
extern Display display;

class Wifi : public Task,
             public HasPreferences {
   public:
    virtual const char* taskName() const override;
    virtual void setup();
    virtual void taskRun() override;

    bool isReady() const;
    void waitForReady();
    void waitForConnection();

    bool isStaConnected() const;
    bool isApConnected() const;
    bool isConnected() const;

    const char* getHostname();
    bool isStaEnabled() const;
    bool isApEnabled() const;
    bool isEnabled() const;
    uint8_t apClientCount() const;
    bool hasApClient() const;

   protected:
    static constexpr const char* ssidKey = "ssid";
    static constexpr const char* passwordKey = "password";
    static constexpr const char* staEnabledKey = "staEnabled";
    static constexpr const char* apEnabledKey = "apEnabled";
    char ssid[64] = {};
    char password[64] = {};
    bool setupDone = false;
    bool staEnabled = false;  // STA    disabled by default
    bool apEnabled = true;    // AP      enabled by default
    bool mdnsStarted = false;

    void setDefaults();
    static void copyString(char* dst, size_t dstSize, const char* src);
    static void trimInPlace(char* text);
    bool loadPreferences();
    void registerApiCommands();
    Api::Reply credentialCommand(const char* args, char* value, size_t valueSize, const char* key);
    Api::Reply wifiCommand(const char* args);
    void startSta();
    void stopSta();
    void restartSta();
    void startAp();
    void stopAP();
    void enableSta();
    void disableSta();
    void enableAP();
    void disableAP();
    void restartAfterModeChange();
    void handleWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info);

    static const char* modeToString(WiFiMode_t mode);
    static const char* statusToString(wl_status_t status);
};

#endif  // WIFI_H