#include "ble_debug.h"

#ifdef FEATURE_DEBUG_BLE

#include <cstdio>
#include <cstring>

void BleDebug::setup(BLEServer* server) {
    BLEService* svc = server->createService(kServiceUuid);
    _logChar = svc->createCharacteristic(
        kLogCharUuid,
        NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ);
    _logChar->setCallbacks(new LogCharCallbacks(this));

    // Initial value so READ returns something before any notify.
    const char* initMsg = "debug log service ready\n";
    _logChar->setValue((const uint8_t*)initMsg, strlen(initMsg));
}

void BleDebug::taskRun() {
    if (!_subscribed || _logChar == nullptr) return;

    const uint32_t now = millis();
    if (now - _lastNotifyMs < kNotifyIntervalMs) return;
    _lastNotifyMs = now;

    // Drain entries from the ring buffer into a batched notification.
    char batch[kMaxBatchBytes];
    size_t batchLen = 0;

    DebugLogEntry entry;
    while (batchLen < sizeof(batch) - sizeof(entry.message) - 1 && g_debugLogBuffer.pop(entry)) {
        int n = snprintf(batch + batchLen, sizeof(batch) - batchLen,
                         "[%lu] %s\n",
                         (unsigned long)entry.timestamp,
                         entry.message);
        if (n > 0 && (size_t)n < sizeof(batch) - batchLen) {
            batchLen += n;
        } else {
            // Message too long, drop it.
            break;
        }
    }

    if (batchLen > 0) {
        _logChar->setValue((const uint8_t*)batch, batchLen);
        _logChar->notify();
    }
}

void BleDebug::LogCharCallbacks::onSubscribe(NimBLECharacteristic* pChar,
                                             NimBLEConnInfo& connInfo,
                                             uint16_t subVal) {
    (void)pChar;
    (void)connInfo;
    _parent->_subscribed = (subVal != 0);
}

#endif  // FEATURE_DEBUG_BLE