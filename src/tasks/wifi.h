#ifndef WIFI_H
#define WIFI_H

/*
 *
 * TODO: Full API interface for configuring WiFi AP/STA.
 *
 * TODO: Manage dependent tasks (OTA, WifiSerial) based on WiFi mode, instead of rebooting on mode change.
 *
 * TODO: Find out why the system sometimes becomes bogged down (barely responsive) when moving out of WiFi range.
 *
 * TODO: Move hostname to main or API task and share it with BLE
 *
 * TODO: Revise API syntax:
 *   - Use "wifi" as the top-level command
 *   - Ommitting the subcommand returns a the current settings: "sta: on, ap: off, ssid: myNetwork, password: myPassword"
 *   - Use "wifi on", "wifi off", "wifi toggle", "wifi ssid", "wifi password", "wifi ap", and "wifi status" as subcommands
 *   - "wifi toggle" toggles the current STA enabled flag, "wifi on" enables STA, "wifi off" disables STA
 *   - "wifi ssid" returns the current STA SSID, "wifi ssid myNetwork" sets the STA SSID
 *   - "wifi password" returns the current STA password, "wifi password myPassword" sets the STA password
 *   - "wifi ap" returns the current AP enabled flag, "wifi ap toggle" toggles AP, "wifi ap on" enables AP, "wifi ap off" disables AP
 *   - "wifi status" returns the current status: "sta: disconnected, ap_clients: 1"
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <Preferences.h>
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
    virtual const char* taskName() const override {
        return "WiFi";
    }

    virtual void setup() {
        ssid = DEFAULT_WIFI_SSID;
        password = DEFAULT_WIFI_PASSWORD;
        hostname = DEFAULT_HOSTNAME;

        if (!preferencesSetup(taskName()))
            ESP_LOGE(taskName(), "Failed to open preferences, using defaults");
        else if (!loadPreferences())
            ESP_LOGE(taskName(), "Failed to load preferences");

        registerApiCommands();

        WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
            handleWiFiEvent(event, info);
        });

        if (!isStaEnabled() && !isApEnabled()) {
            ESP_LOGI(taskName(), "AP and STA are disabled");
        }

        display.menu.onWifiStatusChange(isStaEnabled() ? "WiFi Searching" : "WiFi Disabled");
        display.menu.onWifiApStatusChange(isApEnabled() ? "AP Enabled" : "AP Disabled");

        if (isStaEnabled()) startSta();
        if (isApEnabled()) startAp();

        setupDone = true;
    }

    virtual void taskRun() override {
    }

    bool isReady() const {
        return setupDone;
    }

    void waitForReady() {
        while (!isReady()) {
            ESP_LOGD(taskName(), "Waiting for ready...");
            delay(100);
        }
    }

    void waitForConnection() {
        waitForReady();
        while (!isConnected()) {
            ESP_LOGD(taskName(), "Waiting for connection...");
            delay(100);
        }
    }

    bool isStaConnected() const {
        return WiFi.status() == WL_CONNECTED;
    }
    bool isApConnected() const {
        return hasApClient();
    }

    bool isConnected() const {
        return isStaConnected() || isApConnected();
    }

    const char* getHostname() const {
        return hostname.c_str();
    }

    bool isStaEnabled() const {
        return staEnabled;
    }

    bool isApEnabled() const {
        return apEnabled;
    }

    bool isEnabled() const {
        return isStaEnabled() || isApEnabled();
    }

    String apSsid() const {
        return WiFi.softAPSSID();
    }

    uint8_t apClientCount() const {
        return WiFi.softAPgetStationNum();
    }

    bool hasApClient() const {
        return apClientCount() > 0;
    }

    String getStatusLabel() const {
        char buf[32] = {};
        snprintf(buf, sizeof(buf), "WiFi STA: %s, AP: %s",
                 isStaEnabled() ? isStaConnected() ? "Connected" : "Searching"
                                : "Disabled",
                 isApEnabled() ? apClientCount() > 0 ? "Connected" : "Ready"
                               : "Disabled");
        return String(buf);
    }

   protected:
    static constexpr const char* ssidKey = "ssid";
    static constexpr const char* passwordKey = "password";
    static constexpr const char* hostnameKey = "hostname";
    static constexpr const char* staEnabledKey = "staEnabled";
    static constexpr const char* apEnabledKey = "apEnabled";
    String ssid;
    String password;
    String hostname;
    bool setupDone = false;
    bool staEnabled = false;  // STA    disabled by default
    bool apEnabled = true;    // AP      enabled by default
    bool mdnsStarted = false;

    bool loadPreferences() {
        if (!preferencesReady) {
            ESP_LOGE(taskName(), "Prefs not ready");
            return false;
        }

        staEnabled = preferences.getBool(staEnabledKey, staEnabled);
        apEnabled = preferences.getBool(apEnabledKey, apEnabled);

        if (preferences.isKey(ssidKey) && preferences.isKey(passwordKey)) {
            ssid = preferences.getString(ssidKey, DEFAULT_WIFI_SSID);
            password = preferences.getString(passwordKey, DEFAULT_WIFI_PASSWORD);
        } else {
            ESP_LOGI(taskName(), "Credentials not found in preferences, using defaults (%s:%s)",
                     DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD);
        }

        if (preferences.isKey(hostnameKey))
            hostname = preferences.getString(hostnameKey, DEFAULT_HOSTNAME);
        else
            ESP_LOGI(taskName(), "Hostname not found in preferences, using default: %s", DEFAULT_HOSTNAME);

        ESP_LOGD(taskName(), "staEnabled: %s, apEnabled: %s, ssid: %s, password: %s, hostname: %s",
                 staEnabled ? "true" : "false", apEnabled ? "true" : "false",
                 ssid.c_str(), password.c_str(), hostname.c_str());
        return true;
    }

    void registerApiCommands() {
        api.registerCommand(
            "hostname",
            [this](const char* args) {
                return hostnameCommand(args);
            },
            "Usage: hostname [hostname]\nShows the current hostname, or stores a new hostname when provided.");
        api.registerCommand(
            "wifi",
            [this](const char* args) {
                return wifiCommand(args);
            },
            "Usage: wifi [on|off|toggle]\nToggles or sets WiFi enabled state.");
        api.registerCommand(
            "wifi_ssid",
            [this](const char* args) {
                return credentialCommand(args, ssid, ssidKey);
            },
            "Usage: wifi_ssid [ssid]\nShows the current WiFi SSID, or stores a new SSID when provided.");
        api.registerCommand(
            "wifi_password",
            [this](const char* args) {
                return credentialCommand(args, password, passwordKey);
            },
            "Usage: wifi_password [password]\nShows the current WiFi password, or stores a new password when provided.");
        api.registerCommand(
            "wifi_ap",
            [this](const char* args) {
                return apCommand(args);
            },
            "Usage: wifi_ap [on|off|toggle]\nShows the current WiFi AP mode, or sets it when provided.");
    }

    Api::Reply hostnameCommand(const char* args) {
        Api::Reply reply = {};
        String newValue(args);
        newValue.trim();

        if (newValue.length() > 0 && hostname != newValue) {
            if (!preferencesReady || preferences.putString(hostnameKey, newValue) == 0) {
                ESP_LOGE(taskName(), "Failed to save hostname");
                reply.code = Api::ReplyCode::EXECUTION_ERROR;
                snprintf((char*)reply.data, sizeof(reply.data), "%s", hostname.c_str());
                return reply;
            }
            hostname = newValue;
            if (staEnabled) restartSta();
        }

        snprintf((char*)reply.data, sizeof(reply.data), "%s", hostname.c_str());
        return reply;
    }

    Api::Reply credentialCommand(const char* args, String& value, const char* key) {
        Api::Reply reply = {};
        String newValue(args);
        newValue.trim();

        if (newValue.length() > 0 && value != newValue) {
            if (!preferencesReady || preferences.putString(key, newValue) == 0) {
                ESP_LOGE(taskName(), "Failed to save %s", key);
                reply.code = Api::ReplyCode::EXECUTION_ERROR;
                snprintf((char*)reply.data, sizeof(reply.data), "%s", value.c_str());
                return reply;
            }
            value = newValue;
            if (staEnabled) restartSta();
        }

        snprintf((char*)reply.data, sizeof(reply.data), "%s", value.c_str());
        return reply;
    }

    Api::Reply wifiCommand(const char* args) {
        Api::Reply reply = {};
        String command(args);
        command.trim();

        if (command.length() == 0) {
            // no-op
        } else if (command.equalsIgnoreCase("toggle")) {
            if (staEnabled)
                disableSta();
            else
                enableSta();
        } else if (command.equalsIgnoreCase("on")) {
            enableSta();
        } else if (command.equalsIgnoreCase("off")) {
            disableSta();
        } else {
            reply.code = Api::ReplyCode::INVALID_ARGS;
            snprintf((char*)reply.data, sizeof(reply.data), "Usage: wifi [on|off|toggle]");
            return reply;
        }

        snprintf((char*)reply.data, sizeof(reply.data), "%s", staEnabled ? "enabled" : "disabled");
        return reply;
    }

    Api::Reply apCommand(const char* args) {
        Api::Reply reply = {};
        int8_t newValue = -1;
        if (args != nullptr && args[0] != '\0') {
            if (strcmp(args, "off") == 0)
                newValue = 0;
            else if (strcmp(args, "on") == 0)
                newValue = 1;
            else if (strcmp(args, "toggle") == 0)
                newValue = apEnabled ? 0 : 1;
            else {
                reply.code = Api::ReplyCode::INVALID_ARGS;
                snprintf((char*)reply.data, sizeof(reply.data), "Usage: wifi_ap [on|off|toggle]");
                return reply;
            }
        }

        if (newValue != -1 && newValue != apEnabled) {
            if (newValue == 0)
                disableAP();
            else if (newValue == 1)
                enableAP();
        }

        snprintf((char*)reply.data, sizeof(reply.data), "%s", apEnabled ? "enabled" : "disabled");
        return reply;
    }

    void startSta() {
        if (!staEnabled) return;
        WiFi.setHostname(hostname.c_str());
        WiFi.begin(ssid.c_str(), password.c_str());
        ESP_LOGI(taskName(), "Connecting to WiFi SSID: %s", ssid.c_str());
        if (!mdnsStarted) {
            ESP_LOGI(taskName(), "Starting mDNS with hostname: %s", hostname.c_str());
            if (MDNS.begin(hostname.c_str()))
                mdnsStarted = true;
            else
                ESP_LOGE(taskName(), "Failed to start mDNS");
        }
    }

    void stopSta() {
        if (mdnsStarted) {
            MDNS.end();
            mdnsStarted = false;
        }
        WiFi.disconnect();
    }

    void restartSta() {
        if (!staEnabled) return;
        ESP_LOGI(taskName(), "Restarting STA");
        stopSta();
        startSta();
    }

    void startAp() {
        if (!isApEnabled()) return;
        // Build SSID from hostname + last 4 hex digits of MAC
        String mac = WiFi.macAddress();
        String ssid = hostname + "-" + mac.substring(9, 11) + mac.substring(12, 14);
        if (!WiFi.softAP(ssid.c_str(), nullptr, 0, WIFI_AP_CHANNEL, WIFI_AP_MAX_CONNECTIONS)) {
            ESP_LOGE(taskName(), "Failed to start AP");
            return;
        }
        display.wifiApMode = isApEnabled();
        display.wifiApSsid = apSsid();
        display.queueUiEvent(UiEvent::WifiStatusChange);
        display.menu.onWifiApStatusChange((String("AP: ") + apSsid()).c_str());
        ESP_LOGI(taskName(), "AP started: SSID=%s, IP=%s",
                 apSsid().c_str(), WiFi.softAPIP().toString().c_str());
    }

    void stopAP() {
        if (!isApEnabled()) return;
        WiFi.softAPdisconnect(true);
        display.wifiApMode = isApEnabled();
        display.menu.onWifiApStatusChange("AP Stopped");
        display.queueUiEvent(UiEvent::WifiStatusChange);
        ESP_LOGI(taskName(), "AP stopped");
    }

    void enableSta() {
        if (staEnabled) return;
        staEnabled = true;
        if (!preferencesReady || preferences.putBool(staEnabledKey, staEnabled) == 0) {
            ESP_LOGE(taskName(), "Failed to save enabled state");
            return;
        }
        restartAfterModeChange();
    }

    void disableSta() {
        if (!staEnabled) return;
        staEnabled = false;
        if (!preferencesReady || preferences.putBool(staEnabledKey, staEnabled) == 0) {
            ESP_LOGE(taskName(), "Failed to save STA disabled state");
            return;
        }
        restartAfterModeChange();
    }

    void enableAP() {
        if (apEnabled) return;
        apEnabled = true;
        if (!preferencesReady || preferences.putBool(apEnabledKey, apEnabled) == 0) {
            ESP_LOGE(taskName(), "Failed to save AP enabled state");
            return;
        }
        restartAfterModeChange();
    }

    void disableAP() {
        if (!apEnabled) return;
        apEnabled = false;
        if (!preferencesReady || preferences.putBool(apEnabledKey, apEnabled) == 0) {
            ESP_LOGE(taskName(), "Failed to save AP disabled state");
            return;
        }
        restartAfterModeChange();
    }

    void restartAfterModeChange() {
        ESP_LOGI(taskName(), "Restarting after mode change...");
        wifiSerial.disconnectWithNotice("WiFi mode change: disconnecting wifiSerial session.");
        display.menu.onWifiStatusChange("Restarting...");
        display.menu.onWifiApStatusChange("Restarting...");
        delay(100);
        api.queueCommand("restart");
    }

    void handleWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
        switch (event) {
            case SYSTEM_EVENT_STA_GOT_IP:
                ESP_LOGI(taskName(), "Connected to SSID: %s, IP: %s, Mode: %s",
                         ssid.c_str(), WiFi.localIP().toString().c_str(), modeToString(WiFi.getMode()));
                display.menu.onWifiStatusChange("WiFi Connected");
                display.queueUiEvent(UiEvent::WifiStatusChange);
                if (isApEnabled()) stopAP();
                break;
            case SYSTEM_EVENT_STA_DISCONNECTED:
                ESP_LOGW(taskName(), "Disconnected from SSID: %s, Mode: %s",
                         ssid.c_str(), modeToString(WiFi.getMode()));
                display.menu.onWifiStatusChange("WiFi Disconnected");
                display.queueUiEvent(UiEvent::WifiStatusChange);
                break;
            case SYSTEM_EVENT_AP_STACONNECTED: {
                ESP_LOGI(taskName(), "AP client connected");
                char buf[32] = {};
                snprintf(buf, sizeof(buf), "AP Clients: %d", WiFi.softAPgetStationNum());
                display.menu.onWifiApStatusChange(buf);
                display.queueUiEvent(UiEvent::WifiStatusChange);
                break;
            }
            case SYSTEM_EVENT_AP_STADISCONNECTED: {
                ESP_LOGI(taskName(), "AP client disconnected");
                char buf[32] = {};
                snprintf(buf, sizeof(buf), "AP Clients: %d", WiFi.softAPgetStationNum());
                display.menu.onWifiApStatusChange(buf);
                display.queueUiEvent(UiEvent::WifiStatusChange);
                break;
            }
            default:
                ESP_LOGD(taskName(), "WiFi event: %s", WiFi.eventName(event));
                break;
        }
    }

    static const char* modeToString(WiFiMode_t mode) {
        switch (mode) {
            case WIFI_MODE_NULL:
                return "null";
            case WIFI_MODE_STA:
                return "sta";
            case WIFI_MODE_AP:
                return "ap";
            case WIFI_MODE_APSTA:
                return "apsta";
            default:
                return "unknown";
        }
    }

    static const char* statusToString(wl_status_t status) {
        switch (status) {
            case WL_NO_SHIELD:
                return "no_shield";
            case WL_IDLE_STATUS:
                return "idle";
            case WL_NO_SSID_AVAIL:
                return "no_ssid";
            case WL_SCAN_COMPLETED:
                return "scan_completed";
            case WL_CONNECTED:
                return "connected";
            case WL_CONNECT_FAILED:
                return "connect_failed";
            case WL_CONNECTION_LOST:
                return "connection_lost";
            case WL_DISCONNECTED:
                return "disconnected";
            default:
                return "unknown";
        }
    }
};

#endif  // WIFI_H