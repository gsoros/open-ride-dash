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

extern State state;

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
    }
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
    _bonded = false;
    ESP_LOGI(taskName(), "BLE pairing passkey: %06u", passKey);
    // TODO: display passkey on the fullscreen canvas
}

void Ble::handleAuthenticationComplete() {
    _bonded = true;
    ESP_LOGI(taskName(), "BLE pairing/authentication complete");
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
