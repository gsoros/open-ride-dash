#include "ble.h"

#include <algorithm>
#include <cstring>

#include <NimBLEDevice.h>
#include <esp_random.h>

#include "build_info.h"
#include "model/state.h"
#include "tasks/display.h"
#include "tasks/api.h"

extern State state;
extern Display display;
extern Api api;

/*
    TODO: ESP_LOG* calls are not consistently resulting in output.

    Levels seem OK.

    Increasing NimBLE host stack using -DCONFIG_BT_NIMBLE_HOST_TASK_STACK_SIZE=8192
    or our task with ble.taskStart(1.0f, 8192);
    did not help.

*** first connection _sometimes_ has output: ***
I (7369) BLE: startSecurity
I (7371) BLE: Connected
I (7371) BleSC: onConnect
I (7661) BLE: BLE authentication complete
I (7662) BleSC: onAuthenticationComplete
D (7665) Display: Passkey end event received
*** but no more output on disconnect or subsequent connections ***

*/

namespace {

class BleServerCallbacks : public BLEServerCallbacks {
   public:
    explicit BleServerCallbacks(Ble* ble) : _ble(ble) {}

    void onConnect(BLEServer* server, NimBLEConnInfo& connInfo) override {
        if (_ble != nullptr) _ble->handleConnect(connInfo);
        ESP_LOGI(tag, "onConnect");
    }

    void onDisconnect(BLEServer* server, NimBLEConnInfo& connInfo, int reason) override {
        if (_ble != nullptr) _ble->handleDisconnect(connInfo, reason);
        ESP_LOGI(tag, "onDisconnect");
    }

    uint32_t onPassKeyDisplay() override {
        return _ble != nullptr ? _ble->generatePassKey() : 0;
        ESP_LOGI(tag, "onPassKeyDisplay");
    }

    void onAuthenticationComplete(NimBLEConnInfo& connInfo) override {
        if (_ble != nullptr) _ble->handleAuthenticationComplete(connInfo);
        ESP_LOGI(tag, "onAuthenticationComplete");
    }

   private:
    Ble* _ble;
    const char* tag = "BleSC";
};

// Hardcoded NUS RX payload cap (matches firmware note in ble.h).
constexpr size_t kNusRxMaxLen = 250;

// CTS HR write callback: phone pushes heart rate (uint8 BPM) from a HRM.
class CtsHrCallbacks : public BLECharacteristicCallbacks {
   public:
    explicit CtsHrCallbacks(Ble* ble) : _ble(ble) {}

    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        (void)connInfo;
        if (_ble == nullptr) return;
        const std::string& v = pCharacteristic->getValue();
        if (v.size() >= 1) {
            _ble->onCtsHrWrite(static_cast<uint8_t>(v[0]));
        }
    }

   private:
    Ble* _ble;
};

// NUS RX write callback: phone -> device command line, bridged to Api.
class NusRxCallbacks : public BLECharacteristicCallbacks {
   public:
    explicit NusRxCallbacks(Ble* ble) : _ble(ble) {}

    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        (void)connInfo;
        if (_ble == nullptr) return;
        const std::string& v = pCharacteristic->getValue();
        if (v.empty()) return;
        char line[kNusRxMaxLen + 1] = {};
        size_t n = std::min(v.size(), kNusRxMaxLen);
        std::memcpy(line, v.data(), n);
        line[n] = '\0';
        _ble->onNusRx(line);
    }

   private:
    Ble* _ble;
};
}  // namespace

namespace {
// CTS: custom telemetry service (128-bit UUIDs)
const BLEUUID kCtsServiceUuid("f6333d96-74c0-462d-b92d-5750a2283429");
const BLEUUID kCtsTelemetryCharUuid("5ee460d2-75a3-41ac-9034-2b2d435bb549");
const BLEUUID kCtsHrCharUuid("a2c4f7b1-0e3d-4a8c-9b6e-1f2c3d4e5f60");

// NUS: Nordic UART service (well-known 128-bit UUIDs)
const BLEUUID kNusServiceUuid("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
const BLEUUID kNusRxCharUuid("6e400002-b5a3-f393-e0a9-e50e24dcca9e");  // phone -> device (write)
const BLEUUID kNusTxCharUuid("6e400003-b5a3-f393-e0a9-e50e24dcca9e");  // device -> phone (notify)
}  // namespace

// Fixed-size telemetry payload (little-endian), version 1.
//   byte  0    : version/flags (0x01)
//   bytes 1-2  : speed, km/h * 100 (uint16)
//   bytes 3-4  : battery voltage, V * 100 (uint16)
//   bytes 5-6  : battery current, A * 100 (uint16)
//   byte  7    : state of charge, % (uint8)
//   bytes 8-9  : range, km * 100 (uint16)
//   byte  10   : PAS level (int8: -1 walk, 0 off, 1-5)
//   bytes 11-12: human power, W * 10 (uint16)
//   byte  13   : cadence, RPM (uint8)
constexpr size_t kCtsPayloadSize = Ble::kCtsPayloadSize;

void Ble::setup() {
    if (!preferencesSetup(taskName()))
        ESP_LOGE(taskName(), "Failed to open preferences, using defaults");
    else
        _enabled = preferences.getBool("enabled", _enabled);

    registerApiCommands();

    // Reply queue for NUS-bridged Api commands (TX out to the phone).
    _nusReplyQueue = xQueueCreate(8, sizeof(Api::Reply));
    if (_nusReplyQueue == nullptr) {
        ESP_LOGE(taskName(), "Failed to create NUS reply queue");
    }

    if (!_enabled) {
        ESP_LOGI(taskName(), "BLE is disabled");
        return;
    }

    initializeStack();
    display.queueUiEvent(UiEvent::BleChange);  // Notify display of initial state
}

void Ble::initializeStack() {
    if (_initialized) return;

    BLEDevice::init(state.hostname());

    BLEServer* server = BLEDevice::createServer();
    if (server == nullptr) return;

    server->setCallbacks(new BleServerCallbacks(this));

    _server = server;
    initializeSecurity();

    BLEService* disService = server->createService(BLEUUID((uint16_t)0x180A));
    BLECharacteristic* modelCharacteristic = disService->createCharacteristic(
        BLEUUID((uint16_t)0x2A24), NIMBLE_PROPERTY::READ);
    BLECharacteristic* manufacturerCharacteristic = disService->createCharacteristic(
        BLEUUID((uint16_t)0x2A29), NIMBLE_PROPERTY::READ);
    BLECharacteristic* fwVersionCharacteristic = disService->createCharacteristic(
        BLEUUID((uint16_t)0x2A26), NIMBLE_PROPERTY::READ);

    const char* model = "ORD Dash";
    const char* manufacturer = "OpenRide";
    const char* version = (whoami != nullptr && whoami[0] != '\0') ? whoami : (ord_version != nullptr ? ord_version : "dev");

    modelCharacteristic->setValue(std::string(model));
    manufacturerCharacteristic->setValue(std::string(manufacturer));
    fwVersionCharacteristic->setValue(std::string(version));

    BLEService* basService = server->createService(BLEUUID((uint16_t)0x180F));
    _batteryCharacteristic = basService->createCharacteristic(
        BLEUUID((uint16_t)0x2A19), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    uint8_t initialLevel = 0;
    _batteryCharacteristic->setValue(&initialLevel, 1);

    initializeCyclingServices();
    initializeCtsService();
    initializeNusService();

    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    BLEAdvertisementData advData;
    advData.setName(state.hostname());
    advData.setAppearance(0x0484);
    advData.addServiceUUID(BLEUUID((uint16_t)0x1816));
    advData.addServiceUUID(BLEUUID((uint16_t)0x1818));
    advertising->setAdvertisementData(advData);
    advertising->enableScanResponse(true);
    advertising->start();
    server->start();

    _initialized = true;
}

void Ble::setEnabled(bool enabled) {
    if (enabled == _enabled) return;
    _enabled = enabled;
    if (!preferencesReady || preferences.putBool("enabled", _enabled) == 0) {
        ESP_LOGE(taskName(), "Failed to save enabled state");
        return;
    }
    // The BLE stack cannot be safely started/stopped at runtime without a
    // full re-init, so reboot to apply the change. This mirrors the WiFi
    // task's restartAfterModeChange() behavior.
    ESP_LOGI(taskName(), "BLE %s, restarting to apply", enabled ? "enabled" : "disabled");
    display.queueUiEvent(UiEvent::BleChange);
    delay(100);
    api.queueCommand("restart");
}

void Ble::taskRun() {
    if (!_enabled || !_initialized) return;
    if (_connected) {
        updateBatteryLevel();
        updateCyclingServices();
    }
    // Drain any pending NUS Api replies and forward them out the TX char.
    if (_nusReplyQueue != nullptr) {
        Api::Reply reply;
        while (xQueueReceive(_nusReplyQueue, &reply, 0) == pdTRUE) {
            sendNusReply(reply);
        }
    }
}

void Ble::registerApiCommands() {
    api.registerCommand(
        "ble",
        [this](const char* args) {
            return bleCommand(args);
        },
        "Usage: ble[ on|off|toggle|status]\nShows or sets the BLE enabled state.");
}

Api::Reply Ble::bleCommand(const char* args) {
    Api::Reply reply = {};

    args = Util::skipWhitespace(args);

    if (*args == '\0') {
        // Bare "ble" — return current status summary
        snprintf((char*)reply.data, sizeof(reply.data),
                 "enabled: %s, connected: %s",
                 Util::boolToString(_enabled),
                 Util::boolToString(_connected));
        return reply;
    }

    char sub[16] = {};
    Util::nextToken(args, sub, sizeof(sub));

    if (Util::isStrBoolOn(sub)) {
        setEnabled(true);
        snprintf((char*)reply.data, sizeof(reply.data), "%s", Util::boolToString(_enabled));
    } else if (Util::isStrBoolOff(sub)) {
        setEnabled(false);
        snprintf((char*)reply.data, sizeof(reply.data), "%s", Util::boolToString(_enabled));
    } else if (strcmp(sub, "toggle") == 0) {
        setEnabled(!_enabled);
        snprintf((char*)reply.data, sizeof(reply.data), "%s", Util::boolToString(_enabled));
    } else if (strcmp(sub, "status") == 0) {
        if (*args != '\0') {
            reply.code = Api::Reply::Code::InvalidArgs;
            snprintf((char*)reply.data, sizeof(reply.data), "Usage: ble status");
        } else {
            snprintf((char*)reply.data, sizeof(reply.data),
                     "enabled: %s, connected: %s",
                     Util::boolToString(_enabled),
                     Util::boolToString(_connected));
        }
    } else {
        reply.code = Api::Reply::Code::InvalidArgs;
        snprintf((char*)reply.data, sizeof(reply.data),
                 "Usage: ble[ on|off|toggle|status]");
    }

    return reply;
}

void Ble::initializeCyclingServices() {
    BLEService* cscService = _server->createService(BLEUUID((uint16_t)0x1816));
    _cscCharacteristic = cscService->createCharacteristic(
        BLEUUID((uint16_t)0x2A5B), NIMBLE_PROPERTY::NOTIFY);

    BLEService* cpsService = _server->createService(BLEUUID((uint16_t)0x1818));
    _cpsCharacteristic = cpsService->createCharacteristic(
        BLEUUID((uint16_t)0x2A63), NIMBLE_PROPERTY::NOTIFY);
}

void Ble::updateCyclingServices() {
    if (_server == nullptr || _cscCharacteristic == nullptr || _cpsCharacteristic == nullptr) return;

    uint32_t now = millis();
    if ((now - _lastCyclingPublishMs) < 1000U) return;

    _lastCyclingPublishMs = now;
    publishCscMeasurement();
    publishCpsMeasurement();
    publishCtsMeasurement();
}

void Ble::publishCscMeasurement() {
    /*
        NOTE: The wheeRevs and crankRevs data may be available in one of the unparsed CAN frames,
        it would be nice to use that instead of calculating it here.
    */
    State::Snapshot snapshot = state.getSnapshot(true);
    const uint32_t now = millis();
    const float speedKmph = snapshot.speed();
    const uint8_t cadenceRpm = snapshot.cadence;

    if (_lastCscWheelMs == 0U) {
        _lastCscWheelMs = now;
    }
    if (_lastCscCrankMs == 0U) {
        _lastCscCrankMs = now;
    }

    const uint32_t elapsedWheelMs = now - _lastCscWheelMs;
    const uint32_t elapsedCrankMs = now - _lastCscCrankMs;
    if (elapsedWheelMs > 0U) {
        const float wheelRevPerSecond = (speedKmph * 1000.0f / 3600.0f) / 2.1f;
        const uint32_t addedWheelRevs = static_cast<uint32_t>(wheelRevPerSecond * (elapsedWheelMs / 1000.0f));
        if (addedWheelRevs > 0U) {
            _cscWheelRevolutions += addedWheelRevs;
        }
        _lastCscWheelMs = now;
    }
    if (elapsedCrankMs > 0U) {
        const float crankRevPerSecond = cadenceRpm / 60.0f;
        const uint32_t addedCrankRevs = static_cast<uint32_t>(crankRevPerSecond * (elapsedCrankMs / 1000.0f));
        if (addedCrankRevs > 0U) {
            _cscCrankRevolutions += addedCrankRevs;
        }
        _lastCscCrankMs = now;
    }

    const uint8_t flags = 0x03;
    uint8_t payload[11] = {0};
    payload[0] = flags;

    uint32_t wheelRevs = _cscWheelRevolutions;
    payload[1] = static_cast<uint8_t>((wheelRevs >> 24) & 0xFF);
    payload[2] = static_cast<uint8_t>((wheelRevs >> 16) & 0xFF);
    payload[3] = static_cast<uint8_t>((wheelRevs >> 8) & 0xFF);
    payload[4] = static_cast<uint8_t>(wheelRevs & 0xFF);

    uint16_t wheelEventTime = static_cast<uint16_t>(now / 1000U);
    payload[5] = static_cast<uint8_t>((wheelEventTime >> 8) & 0xFF);
    payload[6] = static_cast<uint8_t>(wheelEventTime & 0xFF);

    uint32_t crankRevs = _cscCrankRevolutions;
    payload[7] = static_cast<uint8_t>((crankRevs >> 24) & 0xFF);
    payload[8] = static_cast<uint8_t>((crankRevs >> 16) & 0xFF);
    payload[9] = static_cast<uint8_t>((crankRevs >> 8) & 0xFF);
    payload[10] = static_cast<uint8_t>(crankRevs & 0xFF);

    _cscCharacteristic->setValue(payload, sizeof(payload));
    _cscCharacteristic->notify();
}

void Ble::publishCpsMeasurement() {
    State::Snapshot snapshot = state.getSnapshot(true);
    const uint16_t power = static_cast<uint16_t>(std::clamp(snapshot.motorPower(), 0.0f, (float)UINT16_MAX));
    if (power == _lastCpsPower) return;

    _lastCpsPower = power;

    uint8_t payload[4] = {0};
    payload[0] = 0x00;
    payload[1] = static_cast<uint8_t>((power >> 8) & 0xFF);
    payload[2] = static_cast<uint8_t>(power & 0xFF);
    payload[3] = 0x00;

    _cpsCharacteristic->setValue(payload, sizeof(payload));
    _cpsCharacteristic->notify();
}

void Ble::initializeCtsService() {
    BLEService* ctsService = _server->createService(kCtsServiceUuid);
    _ctsCharacteristic = ctsService->createCharacteristic(
        kCtsTelemetryCharUuid, NIMBLE_PROPERTY::NOTIFY);
    // HR write char: phone pushes heart rate (uint8 BPM) from a connected HRM.
    _ctsHrCharacteristic = ctsService->createCharacteristic(
        kCtsHrCharUuid, NIMBLE_PROPERTY::WRITE_NR);
    _ctsHrCharacteristic->setCallbacks(new CtsHrCallbacks(this));
}

void Ble::onCtsHrWrite(uint8_t bpm) {
    state.heartRate(bpm);
    ESP_LOGI(taskName(), "CTS HR write: %u BPM", bpm);
    display.queueUiEvent(UiEvent::BleChange);
}

void Ble::initializeNusService() {
    BLEService* nusService = _server->createService(kNusServiceUuid);

    // TX: device -> phone. Notify, no read (Nordic UART convention).
    _nusTxCharacteristic = nusService->createCharacteristic(
        kNusTxCharUuid, NIMBLE_PROPERTY::NOTIFY);

    // RX: phone -> device. Write (with response) so the phone knows we got it.
    _nusRxCharacteristic = nusService->createCharacteristic(
        kNusRxCharUuid, NIMBLE_PROPERTY::WRITE);
    _nusRxCharacteristic->setCallbacks(new NusRxCallbacks(this));
}

void Ble::onNusRx(const char* line) {
    if (line == nullptr || *line == '\0') return;
    ESP_LOGI(taskName(), "NUS RX: '%s'", line);
    // Bridge the line to the existing Api command queue. The Api task
    // processes it and enqueues a reply on our reply queue.
    if (!api.queueCommand(line, _nusReplyQueue)) {
        ESP_LOGW(taskName(), "NUS RX dropped (queue full)");
    }
}

void Ble::sendNusReply(const Api::Reply& reply) {
    if (_server == nullptr || _nusTxCharacteristic == nullptr || !_connected) return;
    char buf[Api::REPLY_SERIAL_LINE_SIZE] = {};
    Api::formatReply(reply, buf, sizeof(buf));
    _nusTxCharacteristic->setValue((uint8_t*)buf, strlen(buf));
    _nusTxCharacteristic->notify();
}

void Ble::publishCtsMeasurement() {
    if (_server == nullptr || _ctsCharacteristic == nullptr) return;

    State::Snapshot snapshot = state.getSnapshot(true);

    const uint16_t speed = snapshot.speed_x100;
    const uint16_t voltage = snapshot.batteryVoltage_x100;
    const uint16_t current = snapshot.batteryCurrent_x100;
    const uint8_t soc = static_cast<uint8_t>(std::clamp(snapshot.soc(), 0.0f, 100.0f));
    const uint16_t range = static_cast<uint16_t>(
        std::clamp(snapshot.range(), 0.0f, (float)UINT16_MAX / 100.0f) * 100.0f);
    const int8_t pasLevel = snapshot.pasLevel;
    const uint16_t humanPower = static_cast<uint16_t>(
        std::clamp(snapshot.humanPower(), 0.0f, (float)UINT16_MAX / 10.0f) * 10.0f);
    const uint8_t cadence = snapshot.cadence;

    uint8_t payload[kCtsPayloadSize] = {0};
    payload[0] = 0x01;  // version
    payload[1] = static_cast<uint8_t>((speed >> 8) & 0xFF);
    payload[2] = static_cast<uint8_t>(speed & 0xFF);
    payload[3] = static_cast<uint8_t>((voltage >> 8) & 0xFF);
    payload[4] = static_cast<uint8_t>(voltage & 0xFF);
    payload[5] = static_cast<uint8_t>((current >> 8) & 0xFF);
    payload[6] = static_cast<uint8_t>(current & 0xFF);
    payload[7] = soc;
    payload[8] = static_cast<uint8_t>((range >> 8) & 0xFF);
    payload[9] = static_cast<uint8_t>(range & 0xFF);
    payload[10] = static_cast<uint8_t>(pasLevel);  // int8 reinterpreted as raw byte
    payload[11] = static_cast<uint8_t>((humanPower >> 8) & 0xFF);
    payload[12] = static_cast<uint8_t>(humanPower & 0xFF);
    payload[13] = cadence;

    // Change-based rate limiting: skip notification when nothing changed.
    if (std::memcmp(payload, _lastCtsPayload, kCtsPayloadSize) == 0) return;
    std::memcpy(_lastCtsPayload, payload, kCtsPayloadSize);

    _ctsCharacteristic->setValue(payload, kCtsPayloadSize);
    _ctsCharacteristic->notify();
}

void Ble::initializeSecurity() {
    NimBLEDevice::setSecurityAuth(/*bonding=*/true, /*mitm=*/true, /*sc=*/true);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
    NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
    NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
}

uint32_t Ble::generatePassKey() {
    uint32_t passKey = (esp_random() + 1) % 1000000U;  // avoid zero
    state.blePassKey(passKey);
    ESP_LOGI(taskName(), "BLE pairing passkey: %06u", passKey);
    display.queueUiEvent(UiEvent::PasskeyStart);
    return passKey;
}

void Ble::handleAuthenticationComplete(NimBLEConnInfo& connInfo) {
    ESP_LOGI(taskName(), "BLE authentication complete");
    state.blePassKey(0);  // reset to invalid passkey
    // TODO: We are sending the PasskeyEnd event after every authentication,
    // even if the passkey was never used (reconnect) or when a wrong passkey
    // was entered. The event name may be confusing.
    display.queueUiEvent(UiEvent::PasskeyEnd);
}

void Ble::handleConnect(NimBLEConnInfo& connInfo) {
    _connected = true;
    NimBLEDevice::stopAdvertising();
    // Require pairing for any further communication
    ESP_LOGI(taskName(), "startSecurity");
    NimBLEDevice::startSecurity(connInfo.getConnHandle());
    updateBatteryLevel();
    ESP_LOGI(taskName(), "Connected");
    display.queueUiEvent(UiEvent::BleChange);
}

void Ble::handleDisconnect(NimBLEConnInfo& connInfo, int reason) {
    _connected = false;
    NimBLEDevice::startAdvertising();
    ESP_LOGI(taskName(), "Disconnected");
    display.queueUiEvent(UiEvent::BleChange);
}

void Ble::updateBatteryLevel() {
    State::Snapshot snapshot = state.getSnapshot(true);
    float soc = snapshot.soc();
    uint8_t newLevel = static_cast<uint8_t>(std::clamp(soc, 0.0f, 100.0f));

    if (newLevel == _batteryLevel && (millis() - _lastBatteryPublishMs) < 1000U) {
        return;
    }

    _batteryLevel = newLevel;
    _lastBatteryPublishMs = millis();

    if (_server == nullptr || _batteryCharacteristic == nullptr) return;

    _batteryCharacteristic->setValue(&_batteryLevel, 1);
    if (_connected) {
        _batteryCharacteristic->notify();
    }
}