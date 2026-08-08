#include "debug_log.h"

#ifdef FEATURE_DEBUG_BLE

#include <Arduino.h>

RingBuffer<DebugLogEntry, 32> g_debugLogBuffer;

void debugLog(const char* fmt, ...) {
    DebugLogEntry entry;
    entry.timestamp = millis();

    va_list args;
    va_start(args, fmt);
    vsnprintf(entry.message, sizeof(entry.message), fmt, args);
    va_end(args);

    // Write to UART too (already goes via ESP_LOG* in most cases, but for
    // explicit debugLog calls we duplicate to ensure capture even if the
    // caller does not also ESP_LOG*).
    // printf("[DEBUG] %s\n", entry.message);

    g_debugLogBuffer.push(entry);
}

#endif  // FEATURE_DEBUG_BLE
