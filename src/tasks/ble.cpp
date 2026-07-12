#include "ble.h"

#include <algorithm>
#include <cstring>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEAdvertising.h>
#include <BLECharacteristic.h>
#include <BLEService.h>
#include <BLE2902.h>

#include "build_info.h"
#include "model/state.h"
#include "tasks/display.h"

extern State state;
extern Display display;

namespace {
class BleServerCallbacks : public BLEServerCallbacks {
   public:
    explicit BleServerCallbacks(Ble* ble) : _ble(ble) {}

    void onConnect(BLEServer* server) override {
        if (_ble != nullptr) _ble->handleConnect();
    }

    void onDisconnect(BLEServer* server) override {
        if (_ble != nullptr) _ble->handleDisconnect();
    }

   private:
    Ble* _ble;
};
}  // namespace

class Ble::SecurityCallbacks : public BLESecurityCallbacks {
   public:
    explicit SecurityCallbacks(Ble* ble) : _ble(ble) {}

    uint32_t onPassKeyRequest() override {
        if (_ble != nullptr) {
            ESP_LOGW(_ble->taskName(), "BLE pairing requested a passkey input from the peer");
        }
        return 0;
    }

    void onPassKeyNotify(uint32_t passKey) override {
        if (_ble != nullptr) {
            _ble->handlePassKeyNotify(passKey);
        }
    }

    bool onSecurityRequest() override {
        return true;
    }

    void onAuthenticationComplete(esp_ble_auth_cmpl_t) override {
        if (_ble != nullptr) {
            _ble->handleAuthenticationComplete();
        }
    }

    bool onConfirmPIN(uint32_t pin) override {
        if (_ble != nullptr) {
            ESP_LOGI(_ble->taskName(), "BLE pairing confirmation for PIN %06u", pin);
        }
        return true;
    }

   private:
    Ble* _ble;
};

void Ble::setup() {
    if (_connected) return;

    BLEDevice::init("ORD Dash");

    BLEServer* server = BLEDevice::createServer();
    if (server == nullptr) return;

    server->setCallbacks(new BleServerCallbacks(this));

    _server = server;
    initializeSecurity();

    BLEService* disService = server->createService(BLEUUID((uint16_t)0x180A));
    BLECharacteristic* modelCharacteristic = disService->createCharacteristic(
        BLEUUID((uint16_t)0x2A24), BLECharacteristic::PROPERTY_READ);
    BLECharacteristic* manufacturerCharacteristic = disService->createCharacteristic(
        BLEUUID((uint16_t)0x2A29), BLECharacteristic::PROPERTY_READ);
    BLECharacteristic* fwVersionCharacteristic = disService->createCharacteristic(
        BLEUUID((uint16_t)0x2A26), BLECharacteristic::PROPERTY_READ);

    const char* model = "ORD Dash";
    const char* manufacturer = "OpenRide";
    const char* version = (whoami != nullptr && whoami[0] != '\0') ? whoami : (ord_version != nullptr ? ord_version : "dev");

    modelCharacteristic->setValue(std::string(model));
    manufacturerCharacteristic->setValue(std::string(manufacturer));
    fwVersionCharacteristic->setValue(std::string(version));

    BLEService* basService = server->createService(BLEUUID((uint16_t)0x180F));
    _batteryCharacteristic = basService->createCharacteristic(
        BLEUUID((uint16_t)0x2A19), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    _batteryCharacteristic->addDescriptor(new BLE2902());
    uint8_t initialLevel = 0;
    _batteryCharacteristic->setValue(&initialLevel, 1);

    initializeCyclingServices();

    disService->start();
    basService->start();

    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    BLEAdvertisementData advData;
    advData.setName("ORD Dash");
    advData.setAppearance(0x0484);
    advertising->setAdvertisementData(advData);
    advertising->addServiceUUID(BLEUUID((uint16_t)0x1816));
    advertising->addServiceUUID(BLEUUID((uint16_t)0x1818));
    advertising->setScanResponse(true);
    advertising->start();
}

void Ble::taskRun() {
    if (_connected) {
        updateBatteryLevel();
        updateCyclingServices();
    }
}

void Ble::initializeCyclingServices() {
    BLEService* cscService = _server->createService(BLEUUID((uint16_t)0x1816));
    _cscCharacteristic = cscService->createCharacteristic(
        BLEUUID((uint16_t)0x2A5B), BLECharacteristic::PROPERTY_NOTIFY);
    _cscCharacteristic->addDescriptor(new BLE2902());

    BLEService* cpsService = _server->createService(BLEUUID((uint16_t)0x1818));
    _cpsCharacteristic = cpsService->createCharacteristic(
        BLEUUID((uint16_t)0x2A63), BLECharacteristic::PROPERTY_NOTIFY);
    _cpsCharacteristic->addDescriptor(new BLE2902());

    cscService->start();
    cpsService->start();
}

void Ble::updateCyclingServices() {
    if (_server == nullptr || _cscCharacteristic == nullptr || _cpsCharacteristic == nullptr) return;

    uint32_t now = millis();
    if ((now - _lastCyclingPublishMs) < 1000U) return;

    _lastCyclingPublishMs = now;
    publishCscMeasurement();
    publishCpsMeasurement();
}

void Ble::publishCscMeasurement() {
    State::Snapshot snapshot = state.getSnapshot(true);
    const uint32_t now = millis();
    const float speedKph = snapshot.speed();
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
        const float wheelRevPerSecond = (speedKph * 1000.0f / 3600.0f) / 2.1f;
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
    const uint16_t power = static_cast<uint16_t>(std::clamp(snapshot.motorPower(), 0.0f, 65535.0f));
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

void Ble::initializeSecurity() {
    if (_securityCallbacks != nullptr) return;

    _securityCallbacks = new SecurityCallbacks(this);
    BLEDevice::setSecurityCallbacks(_securityCallbacks);
    BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_MITM);

    _security.setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);
    _security.setCapability(ESP_IO_CAP_OUT);
    _security.setKeySize(16);
    _security.setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
    _security.setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
}

void Ble::handlePassKeyNotify(uint32_t passKey) {
    _activePassKey = passKey;
    ESP_LOGI(taskName(), "BLE pairing passkey: %06u", passKey);
    state.lastPassKey(passKey);
    display.queueUiEvent(UiEvent::PasskeyStart);
}

void Ble::handleAuthenticationComplete() {
    ESP_LOGI(taskName(), "BLE authentication complete");
    state.lastPassKey(0);
    // TODO: we're sending the passkey end event after every authentication,
    // even if the passkey was never used (reconnect) or when a wrong passkey
    // was entered. The event name may be confusing.
    display.queueUiEvent(UiEvent::PasskeyEnd);
}

void Ble::handleConnect() {
    _connected = true;
    BLEDevice::stopAdvertising();
    updateBatteryLevel();
}

void Ble::handleDisconnect() {
    _connected = false;
    BLEDevice::startAdvertising();
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
