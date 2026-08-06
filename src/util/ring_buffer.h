#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <cstddef>
#include <cstdint>
#include <cstring>

/**
 * @brief Lock-free single-producer single-consumer ring buffer.
 *
 * Thread-safe for one writer (ISR or task) and one reader (task) without
 * mutual exclusion. Uses volatile head/tail with a power-of-two capacity
 * so the index wraps via mask (no modulo).
 *
 * @tparam T  Element type (must be trivially copyable).
 */
template <typename T, size_t Capacity>
class RingBuffer {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of two");
    static_assert(Capacity > 1, "Capacity must be > 1");

   public:
    RingBuffer() : _head(0), _tail(0) {}

    /// Push an element. Overwrites oldest if full. Returns true on success.
    bool push(const T& elem) {
        const size_t h = _head;
        const size_t t = _tail;
        const size_t next = (h + 1) & _mask;
        if (next == t) {
            // Full — overwrite oldest by advancing tail.
            _tail = (t + 1) & _mask;
        }
        _buf[h] = elem;
        // Write barrier: ensure elem is visible before updating head.
        __atomic_store_n(&_head, next, __ATOMIC_RELEASE);
        return true;
    }

    /// Pop an element. Returns false if empty.
    bool pop(T& elem) {
        const size_t t = _tail;
        const size_t h = __atomic_load_n(&_head, __ATOMIC_ACQUIRE);
        if (t == h) return false;
        elem = _buf[t];
        _tail = (t + 1) & _mask;
        return true;
    }

    /// Number of elements currently in the buffer.
    size_t count() const {
        const size_t h = __atomic_load_n(&_head, __ATOMIC_ACQUIRE);
        const size_t t = _tail;
        return (h - t) & _mask;
    }

    /// Maximum capacity.
    static constexpr size_t capacity() { return Capacity; }

    /// Clear all elements.
    void reset() {
        _head = 0;
        _tail = 0;
    }

    /// Index-based read for dump commands (not lock-free! Call only from reader task).
    const T& at(size_t i) const { return _buf[i & _mask]; }
    size_t head() const { return _head; }
    size_t tail() const { return _tail; }

   private:
    static constexpr size_t _mask = Capacity - 1;
    T _buf[Capacity];
    volatile size_t _head = 0;
    volatile size_t _tail = 0;
};

#endif  // RING_BUFFER_H
