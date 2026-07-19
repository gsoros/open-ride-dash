#ifndef SIM_H
#define SIM_H

#ifdef FEATURE_SIM

#include "model/state.h"
#include "task.h"
#include "tasks/api.h"
#include "tasks/alarm.h"
#include "util.h"

extern State state;
extern Api api;
extern Alarm alarmTask;

class NumberSim {
   public:
    NumberSim() {
        unsigned long now = millis();
        lastInjection = now;
        lastUpdate = now;
        nextInterval = (unsigned long)random(100, 501);
        target = (float)random(0, 51);
    }

    void run() {
        unsigned long now = millis();
        isInjectable = false;

        // ---- Physics simulation (continuous, every call ~10ms) ----
        float dt = (now - lastUpdate) / 1000.0f;
        if (dt <= 0.0f) dt = 0.01f;
        if (dt > 0.1f) dt = 0.1f;  // clamp after long pauses
        lastUpdate = now;

        if (currentValue < target) {
            // Accelerate toward target at up to maxAccel unit/h/s
            float accel = maxAccel * dt;
            if (target - currentValue <= accel) {
                currentValue = target;
            } else {
                currentValue += accel;
            }
        } else if (currentValue > target) {
            // Decelerate toward target at up to maxDecel unit/h/s
            float decel = maxDecel * dt;
            if (currentValue - target <= decel) {
                currentValue = target;
            } else {
                currentValue -= decel;
            }
        }

        currentValue = fmax(0.0f, currentValue);

        // ---- Stopping timeout ----
        if (isStopping && (now - stopStartTime >= stopDuration)) {
            isStopping = false;
            target = (float)random(normalMin, normalMax + 1);
        }

        // ---- CAN message injection every minInterval - maxInterval ms ----
        if (now - lastInjection < nextInterval) {
            return;
        }

        isInjectable = true;

        lastInjection = now;
        nextInterval = (unsigned long)random(minInterval, maxInterval + 1);

        if (isStopping) {
            return;
        }

        // stopProbability % chance to "stop"
        if (random(0, 101) < stopProbability) {
            // Enter stopping mode — decelerate to 0 for minStopLength - maxStopLength ms
            isStopping = true;
            stopStartTime = now;
            stopDuration = (unsigned long)random(minStopLength, maxStopLength + 1);
            target = 0.0f;
        } else {
            // burstProbability % chance of a downhill stretch
            if (random(0, 101) < burstProbability) {
                target = (float)random(burstMin, burstMax + 1);
            } else {
                // Normal riding
                target = (float)random(normalMin, normalMax + 1);
            }
        }
    }

    float maxAccel = 5.0f;
    float maxDecel = 10.0f;

    uint16_t normalMin = 15;  // Normal range
    uint16_t normalMax = 40;

    uint16_t minInterval = 100;  // Injection interval
    uint16_t maxInterval = 500;

    uint8_t stopProbability = 1;
    uint16_t minStopLength = 3000;
    uint16_t maxStopLength = 8000;

    uint8_t burstProbability = 25;
    uint16_t burstMin = 50;
    uint16_t burstMax = 60;

    bool isInjectable = false;
    float currentValue = 0.0f;

   protected:
    float target = 0.0f;
    bool isStopping = false;

    // ---- CAN message timing ----
    unsigned long lastInjection = 0;
    unsigned long nextInterval = 100;

    // ---- Physics timing ----
    unsigned long lastUpdate = 0;

    // ---- Stop state ----
    unsigned long stopStartTime = 0;
    unsigned long stopDuration = 2000;
};

// Simulates e-bike activity
class Sim : public Task {
   public:
    virtual const char* taskName() const override {
        return "Sim";
    }

    virtual void setup() {
        randomSeed(esp_random());

        // Speed (km/h) using default values
        _speedSim = new NumberSim();

        // Motor power (Watts)
        _motorPowerSim = new NumberSim();
        _motorPowerSim->normalMin = 200;
        _motorPowerSim->normalMax = 500;
        _motorPowerSim->burstMin = 700;
        _motorPowerSim->burstMax = 900;
        _motorPowerSim->maxAccel = 100.0f;
        _motorPowerSim->maxDecel = 500.0f;
        _motorPowerSim->stopProbability = 2;

        // Human cadence (pedalling rate, RPM)
        _cadenceSim = new NumberSim();
        _cadenceSim->normalMin = 50;  // casual cruising cadence
        _cadenceSim->normalMax = 90;  // typical spinning cadence
        _cadenceSim->burstMin = 90;   // out-of-saddle sprint
        _cadenceSim->burstMax = 110;
        _cadenceSim->maxAccel = 20.0f;  // RPM/s — legs spin up quickly
        _cadenceSim->maxDecel = 40.0f;  // RPM/s — cadence drops when coasting
        _cadenceSim->stopProbability = 2;

        // Human mechanical power (Watts)
        _humanPowerSim = new NumberSim();
        _humanPowerSim->normalMin = 80;     // relaxed rider
        _humanPowerSim->normalMax = 200;    // steady effort
        _humanPowerSim->burstMin = 220;     // hard acceleration
        _humanPowerSim->burstMax = 350;     // sprint
        _humanPowerSim->maxAccel = 80.0f;   // W/s
        _humanPowerSim->maxDecel = 200.0f;  // W/s — power fades fast when easing off
        _humanPowerSim->stopProbability = 2;

        // Start the battery at full charge
        _batteryVoltage = State::BATTERY_CELL_VOLTAGE_MAX * State::BATTERY_PACK_CELL_COUNT;
        _lastBatteryUpdate = millis();

        api.registerCommand("sim", [this](const char* args) { return _simCommand(args); }, "Usage: sim[ on|off]\nSimulates e-bike activity.");
    }

    virtual void taskRun() override {
        if (!_enabled) return;

        // ---- Speed ----
        _speedSim->run();
        if (_speedSim->isInjectable) {
            state.speed_x100(_speedSim->currentValue * 100);
        }

        // ---- Motor power and battery ----
        _motorPowerSim->run();

        // Battery voltage drains proportionally to motor power.
        // Energy used this tick (Wh) = power(W) * dt(s) / 3600, scaled for demo speed.
        // Voltage drop = energyFraction * (fullVoltage - emptyVoltage).
        unsigned long now = millis();
        float dt = (now - _lastBatteryUpdate) / 1000.0f;
        if (dt <= 0.0f) dt = 0.01f;
        if (dt > 0.1f) dt = 0.1f;  // clamp after long pauses
        _lastBatteryUpdate = now;

        float motorW = _motorPowerSim->currentValue;
        float fullV = State::BATTERY_CELL_VOLTAGE_MAX * State::BATTERY_PACK_CELL_COUNT;
        float emptyV = State::BATTERY_CELL_VOLTAGE_MIN * State::BATTERY_PACK_CELL_COUNT;
        float capacityWh = (float)state.batteryCapacity();
        float dWh = motorW * dt / 3600.0f * _batteryDrainScale;
        _batteryVoltage -= dWh / capacityWh * (fullV - emptyV);

        // When empty, jump back to full (simulated "battery swap")
        if (_batteryVoltage <= emptyV) {
            _batteryVoltage = fullV;
        }

        state.batteryVoltage_x100((uint16_t)(_batteryVoltage * 100.0f));
        state.batteryCurrent_x100((uint16_t)(motorW / _batteryVoltage * 100.0f));

        // ---- Human power & cadence ----
        _cadenceSim->run();
        _humanPowerSim->run();
        if (_cadenceSim->isInjectable || _humanPowerSim->isInjectable) {
            float cadenceRpm = _cadenceSim->currentValue;
            float humanW = _humanPowerSim->currentValue;
            state.cadence((uint8_t)cadenceRpm);

            // Derive a realistic raw torque from power = cadence * torqueNm * 2*pi/60
            // torqueNm = power / (cadence * 2*pi/60); raw = torqueNm * TORQUE_NM_FACTOR + 750
            uint16_t torqueRaw;
            if (cadenceRpm < 1.0f) {
                torqueRaw = State::TORQUE_REST_RAW;  // at rest the strain gauge reads its zero offset
            } else {
                float torqueNm = humanW / (cadenceRpm * 0.104719755f);
                torqueRaw = (uint16_t)(torqueNm * State::TORQUE_NM_FACTOR + float(State::TORQUE_REST_RAW));
            }
            state.torque(torqueRaw);
        }

        // Don't let the system go to sleep while the simulator is running
        alarmTask.mock();
    }

    bool isEnabled() const { return _enabled; }

   protected:
    NumberSim* _speedSim = nullptr;
    NumberSim* _motorPowerSim = nullptr;
    NumberSim* _cadenceSim = nullptr;
    NumberSim* _humanPowerSim = nullptr;
    bool _enabled = false;

    float _batteryVoltage = 0.0f;          // live pack voltage (V)
    unsigned long _lastBatteryUpdate = 0;  // for continuous drain timing
    float _batteryDrainScale = 100.0f;     // demo acceleration: higher = faster drain

    Api::Reply _simCommand(const char* args) {
        Api::Reply reply = {};
        if (Util::isStrBoolOn(args))
            _enabled = true;
        else if (Util::isStrBoolOff(args))
            _enabled = false;
        else if (strlen(args) > 0) {
            reply.code = Api::Reply::Code::InvalidArgs;
            snprintf((char*)reply.data, sizeof(reply.data), "Usage: sim[ on|off]");
        }
        snprintf((char*)reply.data, sizeof(reply.data), "sim %s", Util::boolToString(_enabled));
        return reply;
    }
};

#endif  // FEATURE_SIM
#endif  // SIM_H