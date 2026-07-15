#include "tasks/wifi.h"

#include "model/state.h"
extern State state;

#include "tasks/display.h"
extern Display display;

const char* Wifi::taskName() const {
    return "WiFi";
}

void Wifi::setup() {
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

void Wifi::taskRun() {
}

void Wifi::waitForReady() {
    while (!isReady()) {
        ESP_LOGD(taskName(), "Waiting for ready...");
        delay(100);
    }
}

void Wifi::waitForConnection() {
    waitForReady();
    while (!isConnected()) {
        ESP_LOGD(taskName(), "Waiting for connection...");
        delay(100);
    }
}

bool Wifi::isApConnected() const {
    return hasApClient();
}

bool Wifi::isReady() const {
    return setupDone;
}

bool Wifi::isStaConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

bool Wifi::isConnected() const {
    return isStaConnected() || isApConnected();
}

const char* Wifi::getHostname() {
    return state.hostname();
}

bool Wifi::isStaEnabled() const {
    return staEnabled;
}

bool Wifi::isApEnabled() const {
    return apEnabled;
}

bool Wifi::isEnabled() const {
    return isStaEnabled() || isApEnabled();
}

uint8_t Wifi::apClientCount() const {
    return WiFi.softAPgetStationNum();
}

bool Wifi::hasApClient() const {
    return apClientCount() > 0;
}

void Wifi::setDefaults() {
    Util::copyString(ssid, sizeof(ssid), DEFAULT_WIFI_SSID);
    Util::copyString(password, sizeof(password), DEFAULT_WIFI_PASSWORD);
}

bool Wifi::loadPreferences() {
    if (!preferencesReady) {
        ESP_LOGE(taskName(), "Prefs not ready");
        return false;
    }

    staEnabled = preferences.getBool(staEnabledKey, staEnabled);
    apEnabled = preferences.getBool(apEnabledKey, apEnabled);

    if (preferences.isKey(ssidKey) && preferences.isKey(passwordKey)) {
        String storedSsid = preferences.getString(ssidKey, DEFAULT_WIFI_SSID);
        String storedPassword = preferences.getString(passwordKey, DEFAULT_WIFI_PASSWORD);
        Util::copyString(ssid, sizeof(ssid), storedSsid.c_str());
        Util::copyString(password, sizeof(password), storedPassword.c_str());
    } else {
        ESP_LOGI(taskName(), "Credentials not found in preferences, using defaults (%s:%s)",
                 DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD);
    }

    ESP_LOGD(taskName(), "staEnabled: %s, apEnabled: %s, ssid: %s, password: %s",
             staEnabled ? "true" : "false", apEnabled ? "true" : "false",
             ssid, password);
    return true;
}

void Wifi::registerApiCommands() {
    api.registerCommand(
        "wifi",
        [this](const char* args) {
            return wifiCommand(args);
        },
        "Usage: wifi [on|off|toggle|ssid|password|ap|status]\n"
        "  Shows or sets WiFi settings.");
}

Api::Reply Wifi::credentialCommand(const char* args, char* value, size_t valueSize, const char* key) {
    Api::Reply reply = {};
    char newValue[64] = {};
    if (args != nullptr) {
        Util::copyString(newValue, sizeof(newValue), args);
        Util::trimInPlace(newValue);
    }

    if (newValue[0] != '\0' && strcmp(value, newValue) != 0) {
        if (!preferencesReady || preferences.putString(key, newValue) == 0) {
            ESP_LOGE(taskName(), "Failed to save %s", key);
            reply.code = Api::ReplyCode::EXECUTION_ERROR;
            snprintf((char*)reply.data, sizeof(reply.data), "%s", value);
            return reply;
        }
        Util::copyString(value, valueSize, newValue);
        if (staEnabled) restartSta();
    }

    snprintf((char*)reply.data, sizeof(reply.data), "%s", value);
    return reply;
}

Api::Reply Wifi::wifiCommand(const char* args) {
    Api::Reply reply = {};

    args = Util::skipWhitespace(args);

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
    Util::nextToken(args, sub, sizeof(sub));

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
        args = Util::skipWhitespace(args);
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

void Wifi::startSta() {
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

void Wifi::stopSta() {
    if (mdnsStarted) {
        MDNS.end();
        mdnsStarted = false;
    }
    WiFi.disconnect();
}

void Wifi::restartSta() {
    if (!staEnabled) return;
    ESP_LOGI(taskName(), "Restarting STA");
    stopSta();
    startSta();
}

void Wifi::startAp() {
    if (!isApEnabled()) return;
    char apSsid[64] = {};
    Util::copyString(apSsid, sizeof(apSsid), state.hostname());
    // TODO: append last 4 hex digits of MAC to apSsid
    if (!WiFi.softAP(apSsid, nullptr, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CONNECTIONS)) {
        ESP_LOGE(taskName(), "Failed to start AP");
        return;
    }
    display.wifiApMode = isApEnabled();
    Util::copyString(display.wifiApSsid, sizeof(display.wifiApSsid), apSsid);
    display.queueUiEvent(UiEvent::WifiStatusChange);
    display.menu.onWifiApStatusChange((String("AP: ") + apSsid).c_str());
    ESP_LOGI(taskName(), "AP started: SSID=%s, IP=%s",
             apSsid, WiFi.softAPIP().toString().c_str());
}

void Wifi::stopAP() {
    if (!isApEnabled()) return;
    WiFi.softAPdisconnect(true);
    display.wifiApMode = isApEnabled();
    display.menu.onWifiApStatusChange("AP Stopped");
    display.queueUiEvent(UiEvent::WifiStatusChange);
    ESP_LOGI(taskName(), "AP stopped");
}

void Wifi::enableSta() {
    if (staEnabled) return;
    staEnabled = true;
    if (!preferencesReady || preferences.putBool(staEnabledKey, staEnabled) == 0) {
        ESP_LOGE(taskName(), "Failed to save enabled state");
        return;
    }
    restartAfterModeChange();
}

void Wifi::disableSta() {
    if (!staEnabled) return;
    staEnabled = false;
    if (!preferencesReady || preferences.putBool(staEnabledKey, staEnabled) == 0) {
        ESP_LOGE(taskName(), "Failed to save STA disabled state");
        return;
    }
    restartAfterModeChange();
}

void Wifi::enableAP() {
    if (apEnabled) return;
    apEnabled = true;
    if (!preferencesReady || preferences.putBool(apEnabledKey, apEnabled) == 0) {
        ESP_LOGE(taskName(), "Failed to save AP enabled state");
        return;
    }
    restartAfterModeChange();
}

void Wifi::disableAP() {
    if (!apEnabled) return;
    apEnabled = false;
    if (!preferencesReady || preferences.putBool(apEnabledKey, apEnabled) == 0) {
        ESP_LOGE(taskName(), "Failed to save AP disabled state");
        return;
    }
    restartAfterModeChange();
}

void Wifi::restartAfterModeChange() {
    ESP_LOGI(taskName(), "Restarting after mode change...");
    wifiSerial.disconnectWithNotice("WiFi mode change: disconnecting wifiSerial session.");
    display.menu.onWifiStatusChange("Restarting...");
    display.menu.onWifiApStatusChange("Restarting...");
    delay(100);
    api.queueCommand("restart");
}

void Wifi::handleWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
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

const char* Wifi::modeToString(WiFiMode_t mode) {
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

const char* Wifi::statusToString(wl_status_t status) {
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