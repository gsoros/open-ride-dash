#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cstdint>

#ifdef FEATURE_DEBUG_BLE

#include "ring_buffer.h"

/**
 * @brief Debug log entry stored in the ring buffer.
 */
struct DebugLogEntry {
    uint32_t timestamp;  // millis() when logged
    char message[128];   // formatted message (null-terminated)
};

/// Global ring buffer for debug log messages. Capacity must be power of two.
extern RingBuffer<DebugLogEntry, 32> g_debugLogBuffer;

/**
 * @brief Write a debug log message to UART and the ring buffer.
 *
 * Thread-safe for the writer side (single producer — the CAN task).
 * The ring buffer is lock-free SPSC.
 */
void debugLog(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

/// Mark the beginning of a dump section (e.g. on command trigger).
void debugLogDumpStart();

/// Get number of pending entries in the buffer.
inline size_t debugLogCount() { return g_debugLogBuffer.count(); }

#else
// FEATURE_DEBUG_BLE not defined: all debugLog calls compile to nothing.

#define debugLog(...) ((void)0)
#define debugLogDumpStart() ((void)0)
#define debugLogCount() ((size_t)0)

#endif  // FEATURE_DEBUG_BLE

#endif  // DEBUG_LOG_H
