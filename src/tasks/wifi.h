#ifndef WIFI_H
#define WIFI_H

#include <WiFi.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include "task.h"
#include "has_preferences.h"
#include "config.h"
#include "api.h"

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

        if (preferencesSetup(taskName())) {
            if (preferences.isKey(ssidKey) && preferences.isKey(passwordKey)) {
                ssid = preferences.getString(ssidKey, default_wifi_ssid);
                password = preferences.getString(passwordKey, default_wifi_password);
            } else {
                ESP_LOGI(taskName(), "WiFi credentials not found in preferences, using defaults");
            }
            if (preferences.isKey(hostnameKey))
                hostname = preferences.getString(hostnameKey, default_hostname);
            else
                ESP_LOGI(taskName(), "Hostname not found in preferences, using defaults");
        } else
            ESP_LOGE(taskName(), "Failed to open preferences, using defaults");

        registerApiCommands();

        WiFi.mode(WIFI_MODE_STA);
        WiFi.setHostname(hostname.c_str());
        WiFi.begin(ssid.c_str(), password.c_str());
        ESP_LOGI(taskName(), "Connecting to WiFi SSID: %s", ssid.c_str());
        /*
        unsigned long start = millis();
        while (isConnected() && millis() - start < 10000) {
            delay(250);
        }
        if (!isConnected()) {
            ESP_LOGW(taskName(), "WiFi connect failed");
        } else {
            ESP_LOGI(taskName(), "WiFi connected, IP: %s", WiFi.localIP().toString().c_str());
        }
        */
        ESP_LOGI(taskName(), "Starting mDNS with hostname: %s", hostname.c_str());
        MDNS.begin(hostname.c_str());
        setupDone = true;
    }

    virtual void taskRun() override {
        static ulong last = 0;
        if (millis() - last > 10000) {
            last = millis();
            if (!isConnected()) {
                ESP_LOGW(taskName(), "WiFi not connected to '%s', password: '%s'", ssid.c_str(), password.c_str());
            }
        }
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

   protected:
    static constexpr const char* ssidKey = "ssid";
    static constexpr const char* passwordKey = "password";
    static constexpr const char* hostnameKey = "hostname";
    String ssid;
    String password;
    String hostname;
    bool setupDone = false;

    void registerApiCommands() {
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

        if (newValue.length() > 0) {
            if (!preferencesReady || preferences.putString(key, newValue) == 0) {
                reply.code = Api::ReplyCode::EXECUTION_ERROR;
                snprintf((char*)reply.data, sizeof(reply.data), "%s", value.c_str());
                return reply;
            }
            value = newValue;
        }

        snprintf((char*)reply.data, sizeof(reply.data), "%s", value.c_str());
        return reply;
    }

    Api::Reply hostnameCommand(const char* args) {
        Api::Reply reply = {};
        String newValue(args);
        newValue.trim();

        if (newValue.length() > 0) {
            if (!preferencesReady || preferences.putString(hostnameKey, newValue) == 0) {
                reply.code = Api::ReplyCode::EXECUTION_ERROR;
                snprintf((char*)reply.data, sizeof(reply.data), "%s", hostname.c_str());
                return reply;
            }
            hostname = newValue;
        }

        snprintf((char*)reply.data, sizeof(reply.data), "%s", hostname.c_str());
        return reply;
    }
};

#endif  // WIFI_H