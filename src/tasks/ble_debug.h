#ifndef BLE_DEBUG_H
#define BLE_DEBUG_H

#ifdef FEATURE_DEBUG_BLE

#include <Arduino.h>
#include <NimBLEDevice.h>

#include "util/debug_log.h"

/**
 * @brief BLE debug logging service (devel builds only).
 *
 * Exposes a dedicated service with a NOTIFY+READ characteristic that carries
 * formatted debug messages (from explicit debugLog() calls). The phone
 * subscribes to the notify char and accumulates messages.
 *
 * Guarded by FEATURE_DEBUG_BLE — compiles to nothing in prod builds.
 */
class BleDebug {
   public:
    BleDebug() = default;

    /// Create the service + characteristic on the given server.
    void setup(BLEServer* server);

    /// Drain pending log entries and send via NOTIFY. Call from Ble::taskRun().
    void taskRun();

    /// Whether any device is subscribed to the notify char.
    bool hasSubscriber() const { return _subscribed; }

   private:
    static constexpr const char* kServiceUuid = "ad832eae-54b6-4a81-bb49-3125ec77324b";
    static constexpr const char* kLogCharUuid = "ad832eae-54b7-4a81-bb49-3125ec77324b";

    // Rate limiting: max one notify per 50ms.
    static constexpr uint32_t kNotifyIntervalMs = 50;

    // Max payload bytes per notify (well within MTU).
    static constexpr size_t kMaxBatchBytes = 200;

    // Callbacks for subscription tracking.
    class LogCharCallbacks : public BLECharacteristicCallbacks {
       public:
        explicit LogCharCallbacks(BleDebug* parent) : _parent(parent) {}
        void onSubscribe(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo, uint16_t subVal) override;

       private:
        BleDebug* _parent;
    };

    BLECharacteristic* _logChar = nullptr;
    bool _subscribed = false;
    uint32_t _lastNotifyMs = 0;
};

#endif  // FEATURE_DEBUG_BLE
#endif  // BLE_DEBUG_H