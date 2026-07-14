#ifndef WIFI_H
#define WIFI_H

/*
 * TODOs in order of priority:
 *
 *
 *
 *  Find out why the system sometimes becomes bogged down (barely responsive)
 *  when moving out of WiFi range.
 *
 *  Manage dependent tasks (OTA, WifiSerial) based on WiFi mode,
 *  instead of rebooting on mode change?
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
    virtual const char* taskName() const override {
        return "WiFi";
    }

    virtual void setup() {
        setDefaults();

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

    const char* getHostname() {
        return state.hostname();
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

    uint8_t apClientCount() const {
        return WiFi.softAPgetStationNum();
    }

    bool hasApClient() const {
        return apClientCount() > 0;
    }

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

    void setDefaults() {
        copyString(ssid, sizeof(ssid), DEFAULT_WIFI_SSID);
        copyString(password, sizeof(password), DEFAULT_WIFI_PASSWORD);
    }

    static void copyString(char* dst, size_t dstSize, const char* src) {
        if (dst == nullptr || dstSize == 0) return;
        if (src == nullptr) {
            dst[0] = '\0';
            return;
        }
        strncpy(dst, src, dstSize - 1);
        dst[dstSize - 1] = '\0';
    }

    static void trimInPlace(char* text) {
        if (text == nullptr) return;
        char* start = text;
        while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') ++start;
        if (start != text) {
            size_t len = strlen(start) + 1;
            memmove(text, start, len);
        }
        size_t len = strlen(text);
        while (len > 0 && (text[len - 1] == ' ' || text[len - 1] == '\t' || text[len - 1] == '\r' || text[len - 1] == '\n')) {
            text[--len] = '\0';
        }
    }

    bool loadPreferences() {
        if (!preferencesReady) {
            ESP_LOGE(taskName(), "Prefs not ready");
            return false;
        }

        staEnabled = preferences.getBool(staEnabledKey, staEnabled);
        apEnabled = preferences.getBool(apEnabledKey, apEnabled);

        if (preferences.isKey(ssidKey) && preferences.isKey(passwordKey)) {
            String storedSsid = preferences.getString(ssidKey, DEFAULT_WIFI_SSID);
            String storedPassword = preferences.getString(passwordKey, DEFAULT_WIFI_PASSWORD);
            copyString(ssid, sizeof(ssid), storedSsid.c_str());
            copyString(password, sizeof(password), storedPassword.c_str());
        } else {
            ESP_LOGI(taskName(), "Credentials not found in preferences, using defaults (%s:%s)",
                     DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD);
        }

        ESP_LOGD(taskName(), "staEnabled: %s, apEnabled: %s, ssid: %s, password: %s",
                 staEnabled ? "true" : "false", apEnabled ? "true" : "false",
                 ssid, password);
        return true;
    }

    void registerApiCommands() {
        api.registerCommand(
            "wifi",
            [this](const char* args) {
                return wifiCommand(args);
            },
            "Usage: wifi [on|off|toggle|ssid|password|ap|status]\n"
            "  Shows or sets WiFi settings.");
    }

    Api::Reply credentialCommand(const char* args, char* value, size_t valueSize, const char* key) {
        Api::Reply reply = {};
        char newValue[64] = {};
        if (args != nullptr) {
            strncpy(newValue, args, sizeof(newValue) - 1);
            trimInPlace(newValue);
        }

        if (newValue[0] != '\0' && strcmp(value, newValue) != 0) {
            if (!preferencesReady || preferences.putString(key, newValue) == 0) {
                ESP_LOGE(taskName(), "Failed to save %s", key);
                reply.code = Api::ReplyCode::EXECUTION_ERROR;
                snprintf((char*)reply.data, sizeof(reply.data), "%s", value);
                return reply;
            }
            copyString(value, valueSize, newValue);
            if (staEnabled) restartSta();
        }

        snprintf((char*)reply.data, sizeof(reply.data), "%s", value);
        return reply;
    }

    Api::Reply wifiCommand(const char* args) {
        Api::Reply reply = {};

        // Skip leading whitespace
        while (*args == ' ' || *args == '\t') args++;

        if (*args == '\0') {
            // Bare "wifi" — return current settings summary
            snprintf((char*)reply.data, sizeof(reply.data),
                     "sta: %s, ap: %s, ssid: %s, password: %s",
                     staEnabled ? "on" : "off",
                     apEnabled ? "on" : "off",
                     ssid, password);
            return reply;
        }

        // Extract the subcommand
        char sub[16] = {};
        size_t i = 0;
        while (*args && *args != ' ' && *args != '\t' && i < sizeof(sub) - 1) {
            sub[i++] = *args++;
        }
        sub[i] = '\0';
        while (*args == ' ' || *args == '\t') args++;

        if (strcmp(sub, "on") == 0) {
            enableSta();
            snprintf((char*)reply.data, sizeof(reply.data), "%s", staEnabled ? "enabled" : "disabled");
        } else if (strcmp(sub, "off") == 0) {
            disableSta();
            snprintf((char*)reply.data, sizeof(reply.data), "%s", staEnabled ? "enabled" : "disabled");
        } else if (strcmp(sub, "toggle") == 0) {
            if (staEnabled)
                disableSta();
            else
                enableSta();
            snprintf((char*)reply.data, sizeof(reply.data), "%s", staEnabled ? "enabled" : "disabled");
        } else if (strcmp(sub, "ssid") == 0) {
            return credentialCommand(args, ssid, sizeof(ssid), ssidKey);
        } else if (strcmp(sub, "password") == 0) {
            return credentialCommand(args, password, sizeof(password), passwordKey);
        } else if (strcmp(sub, "ap") == 0) {
            while (*args == ' ' || *args == '\t') args++;
            if (*args == '\0') {
                snprintf((char*)reply.data, sizeof(reply.data), "%s", apEnabled ? "enabled" : "disabled");
            } else if (strcmp(args, "on") == 0) {
                enableAP();
                snprintf((char*)reply.data, sizeof(reply.data), "%s", apEnabled ? "enabled" : "disabled");
            } else if (strcmp(args, "off") == 0) {
                disableAP();
                snprintf((char*)reply.data, sizeof(reply.data), "%s", apEnabled ? "enabled" : "disabled");
            } else if (strcmp(args, "toggle") == 0) {
                if (apEnabled)
                    disableAP();
                else
                    enableAP();
                snprintf((char*)reply.data, sizeof(reply.data), "%s", apEnabled ? "enabled" : "disabled");
            } else {
                reply.code = Api::ReplyCode::INVALID_ARGS;
                snprintf((char*)reply.data, sizeof(reply.data), "Usage: wifi ap [on|off|toggle]");
            }
        } else if (strcmp(sub, "status") == 0) {
            if (*args != '\0') {
                reply.code = Api::ReplyCode::INVALID_ARGS;
                snprintf((char*)reply.data, sizeof(reply.data), "Usage: wifi status (no arguments)");
            } else {
                snprintf((char*)reply.data, sizeof(reply.data),
                         "sta: %s, ap_clients: %d",
                         isStaConnected() ? "connected" : "disconnected",
                         apClientCount());
            }
        } else {
            reply.code = Api::ReplyCode::INVALID_ARGS;
            snprintf((char*)reply.data, sizeof(reply.data),
                     "Usage: wifi [on|off|toggle|ssid|password|ap|status]");
        }

        return reply;
    }

    void startSta() {
        if (!staEnabled) return;
        WiFi.setHostname(state.hostname());
        WiFi.begin(ssid, password);
        ESP_LOGI(taskName(), "Connecting to WiFi SSID: %s", ssid);
        if (!mdnsStarted) {
            ESP_LOGI(taskName(), "Starting mDNS with hostname: %s", state.hostname());
            if (MDNS.begin(state.hostname()))
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

   protected:
    void restartSta() {
        if (!staEnabled) return;
        ESP_LOGI(taskName(), "Restarting STA");
        stopSta();
        startSta();
    }

    void startAp() {
        if (!isApEnabled()) return;
        char apSsid[64] = {};
        copyString(apSsid, sizeof(apSsid), state.hostname());
        // TODO: append last 4 hex digits of MAC to apSsid
        if (!WiFi.softAP(apSsid, nullptr, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CONNECTIONS)) {
            ESP_LOGE(taskName(), "Failed to start AP");
            return;
        }
        display.wifiApMode = isApEnabled();
        copyString(display.wifiApSsid, sizeof(display.wifiApSsid), apSsid);
        display.queueUiEvent(UiEvent::WifiStatusChange);
        display.menu.onWifiApStatusChange((String("AP: ") + apSsid).c_str());
        ESP_LOGI(taskName(), "AP started: SSID=%s, IP=%s",
                 apSsid, WiFi.softAPIP().toString().c_str());
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
                         ssid, WiFi.localIP().toString().c_str(), modeToString(WiFi.getMode()));
                display.menu.onWifiStatusChange("WiFi Connected");
                display.queueUiEvent(UiEvent::WifiStatusChange);
                break;
            case SYSTEM_EVENT_STA_DISCONNECTED:
                ESP_LOGW(taskName(), "Disconnected from SSID: %s, Mode: %s",
                         ssid, modeToString(WiFi.getMode()));
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