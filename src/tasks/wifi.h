#ifndef WIFI_H
#define WIFI_H

/*
 * TODO: Fallback AP support and full API interface for configuring WiFi AP/STA
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include "task.h"
#include "has_preferences.h"
#include "config.h"
#include "api.h"
#include "tasks/telnet.h"

extern Telnet telnet;

class Wifi : public Task,
             public HasPreferences {
   public:
    virtual const char* taskName() override {
        return "WiFi";
    }

    virtual void setup() {
        ssid = default_wifi_ssid;
        password = default_wifi_password;
        hostname = default_hostname;

        if (!preferencesSetup(taskName()))
            ESP_LOGE(taskName(), "Failed to open preferences, using defaults");
        else if (!loadPreferences())
            ESP_LOGE(taskName(), "Failed to load preferences");

        registerApiCommands();

        if (staEnabled) {
            startSTA();
        } else {
            ESP_LOGI(taskName(), "STA is disabled");
            WiFi.mode(WIFI_MODE_NULL);
        }

        setupDone = true;
    }

    virtual void taskRun() override {
        if (!isEnabled()) return;
        static ulong last = 0;
        if (millis() - last > 10000) {
            last = millis();
            if (!isConnected()) {
                ESP_LOGW(taskName(), "Not connected to '%s', password: '%s'", ssid.c_str(), password.c_str());
            }
        }
    }

    bool loadPreferences() {
        if (!preferencesReady) {
            ESP_LOGE(taskName(), "Prefs not ready");
            return false;
        }
        staEnabled = preferences.getBool(staEnabledKey, true);
        if (preferences.isKey(ssidKey) && preferences.isKey(passwordKey)) {
            ssid = preferences.getString(ssidKey, default_wifi_ssid);
            password = preferences.getString(passwordKey, default_wifi_password);
        } else {
            ESP_LOGI(taskName(), "Credentials not found in preferences, using defaults");
        }
        if (preferences.isKey(hostnameKey))
            hostname = preferences.getString(hostnameKey, default_hostname);
        else
            ESP_LOGI(taskName(), "Hostname not found in preferences, using defaults");
        return true;
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

    bool isConnected() const {
        return WiFi.status() == WL_CONNECTED;
    }

    const char* getHostname() const {
        return hostname.c_str();
    }

    bool isEnabled() const {
        return staEnabled;
    }

    const char* getStatusLabel() const {
        if (!staEnabled) return "WiFi (disabled)";
        if (!setupDone) return "WiFi (starting)";
        return isConnected() ? "WiFi (connected)" : "WiFi (searching)";
    }

   protected:
    static constexpr const char* ssidKey = "ssid";
    static constexpr const char* passwordKey = "password";
    static constexpr const char* hostnameKey = "hostname";
    static constexpr const char* staEnabledKey = "staEnabled";
    String ssid;
    String password;
    String hostname;
    bool setupDone = false;
    bool staEnabled = true;
    bool mdnsStarted = false;

    void registerApiCommands() {
        api.registerCommand(
            "wifi",
            [this](const char* args) {
                return wifiCommand(args);
            },
            "Usage: wifi [on|off]\nToggles or sets WiFi enabled state.");
        api.registerCommand(
            "wifi_ssid",
            [this](const char* args) {
                return wifiCredentialCommand(args, ssid, ssidKey);
            },
            "Usage: wifi_ssid [ssid]\nShows the current WiFi SSID, or stores a new SSID when provided.");
        api.registerCommand(
            "wifi_password",
            [this](const char* args) {
                return wifiCredentialCommand(args, password, passwordKey);
            },
            "Usage: wifi_password [password]\nShows the current WiFi password, or stores a new password when provided.");
        api.registerCommand(
            "hostname",
            [this](const char* args) {
                return hostnameCommand(args);
            },
            "Usage: hostname [hostname]\nShows the current hostname, or stores a new hostname when provided.");
    }

    Api::Reply wifiCredentialCommand(const char* args, String& value, const char* key) {
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
            if (staEnabled) restartSTA();
        }

        snprintf((char*)reply.data, sizeof(reply.data), "%s", value.c_str());
        return reply;
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
            if (staEnabled) restartSTA();
        }

        snprintf((char*)reply.data, sizeof(reply.data), "%s", hostname.c_str());
        return reply;
    }

    Api::Reply wifiCommand(const char* args) {
        Api::Reply reply = {};
        String command(args);
        command.trim();

        if (command.length() == 0) {
            if (staEnabled)
                disableSTA();
            else
                enableSTA();
        } else if (command.equalsIgnoreCase("on") || command.equalsIgnoreCase("enable")) {
            enableSTA();
        } else if (command.equalsIgnoreCase("off") || command.equalsIgnoreCase("disable")) {
            disableSTA();
        } else {
            reply.code = Api::ReplyCode::INVALID_ARGS;
            snprintf((char*)reply.data, sizeof(reply.data), "Usage: wifi [on|off]");
            return reply;
        }

        if (preferencesReady && preferences.putBool(staEnabledKey, staEnabled) == 0) {
            ESP_LOGE(taskName(), "Failed to save WiFi enabled state");
            reply.code = Api::ReplyCode::EXECUTION_ERROR;
        }

        snprintf((char*)reply.data, sizeof(reply.data), "%s", staEnabled ? "enabled" : "disabled");
        return reply;
    }

    void startSTA() {
        WiFi.mode(WIFI_MODE_STA);
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

    void stopSTA() {
        if (mdnsStarted) {
            MDNS.end();
            mdnsStarted = false;
        }
        WiFi.disconnect(true);
        WiFi.mode(WIFI_MODE_NULL);
    }

    void enableSTA() {
        staEnabled = true;
        if (!preferencesReady || preferences.putBool(staEnabledKey, staEnabled) == 0)
            ESP_LOGE(taskName(), "Failed to save STA enabled state");
        startSTA();
    }

    void disableSTA() {
        ESP_LOGI(taskName(), "Disabling STA");
        telnet.disconnectWithNotice("WiFi STA disabled: disconnecting telnet session.");
        staEnabled = false;
        if (!preferencesReady || preferences.putBool(staEnabledKey, staEnabled) == 0)
            ESP_LOGE(taskName(), "Failed to save STA disabled state");
        stopSTA();
    }

    void restartSTA() {
        if (!staEnabled) return;
        stopSTA();
        startSTA();
        ESP_LOGI(taskName(), "Restarting STA connection to %s", ssid.c_str());
    }
};

#endif  // WIFI_H