#ifndef BLE_H
#define BLE_H

/*
    BLE (GAP Peripheral, GATT Server) task.

    IMPLEMENTATION PLAN:

    Goals:
    - Provide basic telemetry to mobile fitness apps using standard services. (CSC, CPS)
    - Provide richer telemetry to the custom mobile app. (CTS)
    - Provide an API bridge to the custom mobile app via NUS once the app is ready. (NUS)

    Constraints and policies:
    - Stack: NimBLE (h2zero/NimBLE-Arduino), not the bundled Bluedroid BLE lib.
    - Maximum connections: 1. Advertising stops immediately upon connection.
      NimBLE defaults to 3 max connections; cap it via a build flag
      (-DCONFIG_BT_NIMBLE_MAX_CONNECTIONS=1) if the RAM saving matters, since
      the app-level stop-advertising-on-connect logic alone won't shrink the
      stack's connection pool.
    - Security: use authenticated pairing: display generated passkey.
    - Advertising payload should fit the legacy 31-byte limit and include flags, appearance,
      and the mandatory service UUIDs for CSC/CPS compatibility.
    - Minimize BLE stack footprint, memory use, and fragmentation.
    - Avoid blocking.
    - Avoid race conditions.

    Service plan:
    1. DIS + BAS [Status: DONE]
       - Implement Device Information Service and Battery Service first.
       - Add basic advertising and connection handling.
       - Validate pairing behavior and the initial security model early.

    2. CSC + CPS [Status: DONE, not yet tested]
       - Add Cycling Speed & Cadence and Cycling Power services.
       - Send updates at a low rate, typically 1-2 Hz max, and only when values change.
       - Keep the rolling counter logic simple and deterministic.

    3. CTS [Status: DONE]
       - DONE: Add a compact custom telemetry service for the mobile app.
       - DONE: Use a fixed-size payload derived from the bike state snapshot.
       - DONE: Apply the same change-based rate limiting as the standard services.
       - DONE: Heart rate char (write) so the phone can push HR data to the display.

    4. NUS [Status: DONE]
       - DONE: Nordic UART service (RX write / TX notify).
       - DONE: RX bridged to the existing Api command queue (hostname, battery,
         ble, wifi, restart, v, help, ...). TX carries formatted Api replies.
       - DONE: Hardcoded 250-byte RX cap; TX replies fragmented over NOTIFY; first
         frame carries a 2-byte big-endian total-length prefix, subsequent frames
         are raw continuation data; MTU negotiated to 247.

    GATT profile:
    - DIS: 0x180A, read-only device information.
    - CSC: 0x1816, measurement characteristic 0x2A5B, notify.
    - CPS: 0x1818, measurement characteristic 0x2A63, notify.
    - BAS: 0x180F, level characteristic 0x2A19, read/notify.
    - CTS: custom 128-bit service with a telemetry characteristic (notify) and a HR char (write).
    - NUS: Nordic UART service with RX/TX characteristics, write for RX and
      notify (fragmented, length-prefixed) for TX. RX lines are bridged to the
      existing Api command queue; TX carries Api replies.

    Implementation notes:
    - The BLE task owns the BLE state machine, connection lifecycle, and outgoing notifications.
    - Work from a fixed snapshot of the bike state rather than reading live state directly in several places.
    - Keep notification paths non-blocking, bounded, and based on fixed-size buffers.
    - Use a small state machine: init -> advertise -> connected -> secured -> ready -> disconnect.
    - Prefer change detection and coalescing over sending every update.
    - Avoid blocking the task with long serialization or large buffers.
*/

#include <NimBLEDevice.h>

#include "task.h"
#include "has_preferences.h"
#include "api.h"

class Ble : public Task,
            public HasPreferences {
   public:
    virtual const char* taskName() const override {
        return "BLE";
    }

    virtual void setup();
    virtual void taskRun() override;

    void handleConnect(NimBLEConnInfo& connInfo);
    void handleDisconnect(NimBLEConnInfo& connInfo, int reason);
    void handleAuthenticationComplete(NimBLEConnInfo& connInfo);
    void handleMtuChange(uint16_t mtu);
    void onCtsHrWrite(uint8_t bpm);
    void onNusRx(const char* line);

    uint32_t generatePassKey();

    // BLE enabled state. When false, the BLE stack is not initialized and
    // must not be accessed. Persisted via preferences.
    bool isEnabled() const { return _enabled; }
    void setEnabled(bool enabled);
    bool isConnected() const { return _connected; }

   private:
    void updateBatteryLevel();
    void updateCyclingServices();
    void initializeSecurity();
    void initializeCyclingServices();
    void publishCscMeasurement();
    void publishCpsMeasurement();
    void initializeCtsService();
    void publishCtsMeasurement();
    void initializeNusService();
    void sendNusReply(const Api::Reply& reply);
    void initializeStack();
    void registerApiCommands();
    Api::Reply bleCommand(const char* args);

    BLEServer* _server = nullptr;
    BLECharacteristic* _batteryCharacteristic = nullptr;
    BLECharacteristic* _cscCharacteristic = nullptr;
    BLECharacteristic* _cpsCharacteristic = nullptr;
    BLECharacteristic* _ctsCharacteristic = nullptr;
    BLECharacteristic* _ctsHrCharacteristic = nullptr;
    BLECharacteristic* _nusRxCharacteristic = nullptr;
    BLECharacteristic* _nusTxCharacteristic = nullptr;

    bool _enabled = false;  // BLE disabled by default
    bool _initialized = false;
    bool _connected = false;
    uint16_t _mtu = 23;                      // negotiated ATT MTU (default 23 until onMTUChange)
    QueueHandle_t _nusReplyQueue = nullptr;  // Api replies for NUS TX
    uint8_t _batteryLevel = 0;
    uint32_t _lastBatteryPublishMs = 0;
    uint32_t _lastCyclingPublishMs = 0;
    uint32_t _cscWheelRevolutions = 0;
    uint16_t _cscLastWheelEventTime = 0;
    uint32_t _cscCrankRevolutions = 0;
    uint16_t _cscLastCrankEventTime = 0;
    uint32_t _lastCscWheelMs = 0;
    uint32_t _lastCscCrankMs = 0;
    uint16_t _lastCpsPower = 0;

   public:
    static constexpr size_t kCtsPayloadSize = 14;

   private:
    uint8_t _lastCtsPayload[kCtsPayloadSize] = {};
};

#endif  // BLE_H