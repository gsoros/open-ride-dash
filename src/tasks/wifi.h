#include <WiFi.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include "task.h"
#include "config.h"
#include "api.h"

class Wifi : public Task {
   public:
    virtual const char* taskName() override {
        return "WiFi";
    }

    virtual void setup() {
        setupPreferences();
        registerApiCommands();

        WiFi.mode(WIFI_MODE_STA);
        WiFi.begin(ssid.c_str(), password.c_str());
        ESP_LOGI(taskName(), "Connecting to WiFi SSID: %s", ssid.c_str());
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
            delay(250);
        }
        if (WiFi.status() != WL_CONNECTED) {
            ESP_LOGW(taskName(), "WiFi connect failed");
        } else {
            ESP_LOGI(taskName(), "WiFi connected, IP: %s", WiFi.localIP().toString().c_str());
        }
        ESP_LOGI(taskName(), "Starting mDNS with hostname: %s", default_hostname);
        MDNS.begin(default_hostname);
        Task::taskSetup();
    }

    virtual void taskRun() override {
        // ESP_LOGD(taskName(), "Status: %d Heap: %u Stack: %d", WiFi.status(), xPortGetFreeHeapSize(), taskGetLowestStackLevel());
    }

    bool isReady() const {
        return taskSetupDone;
    }

    void waitForConnection() {
        while (!isReady()) {
            ESP_LOGD(taskName(), "Waiting for setup to complete...");
            delay(100);
        }
    }

   protected:
    static constexpr const char* preferencesNamespace = "wifi";
    static constexpr const char* ssidKey = "ssid";
    static constexpr const char* passwordKey = "password";

    Preferences preferences;
    String ssid;
    String password;
    bool preferencesReady = false;

    void setupPreferences() {
        preferencesReady = preferences.begin(preferencesNamespace, false);
        if (!preferencesReady) {
            ESP_LOGE(taskName(), "Failed to open WiFi preferences, using defaults");
            ssid = default_wifi_ssid;
            password = default_wifi_password;
            return;
        }

        if (!preferences.isKey(ssidKey) || !preferences.isKey(passwordKey)) {
            ESP_LOGI(taskName(), "WiFi credentials not found in preferences, using defaults");
            ssid = default_wifi_ssid;
            password = default_wifi_password;
            return;
        }
        ssid = preferences.getString(ssidKey, default_wifi_ssid);
        password = preferences.getString(passwordKey, default_wifi_password);
    }

    void registerApiCommands() {
        api.registerCommand("wifi_ssid", [this](const char* args) {
            return wifiCredentialCommand(args, ssid, ssidKey);
        });
        api.registerCommand("wifi_password", [this](const char* args) {
            return wifiCredentialCommand(args, password, passwordKey);
        });
    }

    Api::Reply wifiCredentialCommand(const char* args, String& value, const char* key) {
        Api::Reply reply = {};
        String newValue(args);
        newValue.trim();

        if (newValue.length() > 0) {
            if (!preferencesReady || preferences.putString(key, newValue) == 0) {
                reply.errorCode = Api::ErrorCode::EXECUTION_ERROR;
                snprintf((char*)reply.data, sizeof(reply.data), "%s", value.c_str());
                return reply;
            }
            value = newValue;
        }

        snprintf((char*)reply.data, sizeof(reply.data), "%s", value.c_str());
        return reply;
    }
};
