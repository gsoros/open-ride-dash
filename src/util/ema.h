#ifndef EMA_H
#define EMA_H

/**
 * @brief Simple exponential moving average filter.
 *
 * Usage:
 *   Ema ema(0.2f);
 *   float smoothed = ema.filter(rawValue);
 *   float current = ema.value();  // last filtered value
 *   ema.reset();  // re-initialise (next filter() seeds from raw)
 *   ema.setAlpha(0.1f);  // change smoothing factor mid-run
 */
class Ema {
   public:
    explicit Ema(float alpha, bool bypass = false)
        : _value(0.0f), _alpha(alpha), _bypass(bypass), _initialized(false) {}

    /// Feed a raw sample; returns the filtered value (or raw if bypass is set).
    float filter(float raw) {
        if (_bypass || !_initialized) {
            _value = raw;
            _initialized = true;
            return raw;
        }
        _value = _alpha * raw + (1.0f - _alpha) * _value;
        return _value;
    }

    /// Current filtered value.
    float value() const { return _value; }

    /// Reset the filter — next filter() seeds from raw.
    void reset() { _initialized = false; }

    /// Change smoothing factor (0 < alpha ≤ 1). Larger = more responsive.
    void setAlpha(float alpha) { _alpha = alpha; }

    /// Enable/disable bypass. When bypassed, filter() returns the raw value.
    void setBypass(bool bypass) { _bypass = bypass; }
    bool isBypassed() const { return _bypass; }

   private:
    float _value;
    float _alpha;
    bool _bypass;
    bool _initialized;
};

#endif  // EMA_H
