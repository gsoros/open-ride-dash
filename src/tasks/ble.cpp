#include "ble.h"

#include <algorithm>
#include <cstring>

#include <NimBLEDevice.h>
#include <esp_random.h>

#include "build_info.h"
#include "model/state.h"
#include "tasks/display.h"

extern State state;
extern Display display;

/*
     TODO: ESP_LOG* calls are not consistently resulting in output.

     Levels seem OK.

     Increasing stack using
     -DCONFIG_BT_NIMBLE_HOST_TASK_STACK_SIZE=8192
       or
     ble.taskStart(1.0f, 8192);
     do not help.

*** first connection sometimes has output: ***
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
}  // namespace

void Ble::setup() {
    if (_connected) return;

    // TODO: Use a shared hostname across WiFi and BLE

    BLEDevice::init("ORD Dash");

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

    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    BLEAdvertisementData advData;
    advData.setName("ORD Dash");
    advData.setAppearance(0x0484);
    advData.addServiceUUID(BLEUUID((uint16_t)0x1816));
    advData.addServiceUUID(BLEUUID((uint16_t)0x1818));
    advertising->setAdvertisementData(advData);
    advertising->enableScanResponse(true);
    advertising->start();
    server->start();
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
}

void Ble::handleDisconnect(NimBLEConnInfo& connInfo, int reason) {
    _connected = false;
    NimBLEDevice::startAdvertising();
    ESP_LOGI(taskName(), "Disconnected");
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