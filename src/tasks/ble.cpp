#include "ble.h"
/*

    BLE Task Implementation Ideas


    CSC:

    typedef struct __attribute__((packed)) {
        uint8_t flags;                 // Bit 0: Wheel Rev Data Present (1), Bit 1: Crank Rev Data Present (1)
        uint32_t cumulative_wheel_revs;// Total wheel revolutions
        uint16_t last_wheel_event_time;// Time of last wheel event (1/1024 seconds)
        uint16_t cumulative_crank_revs;// Total crank revolutions (pedal rotations)
        uint16_t last_crank_event_time;// Time of last crank event (1/1024 seconds)
    } csc_measurement_t;

    uint32_t ble_cumulative_wheel_revs = 0;
    uint16_t ble_last_wheel_event_time = 0;
    uint16_t ble_cumulative_crank_revs = 0;
    uint16_t ble_last_crank_event_time = 0;

    void update_csc_counters(const State::Snapshot& s, uint32_t delta_ms) {
        float dt = delta_ms / 1000.0f;

        if (s.speed_x100 > 0) {
            float speed_mps = (s.speed_x100 / 100.0f) / 3.6f; // km/h to m/s
            float circ_meters = s.wheelCircumference / 1000.0f;
            float wheel_revs_per_sec = speed_mps / circ_meters;
            static float wheel_fractional = 0.0f;
            wheel_fractional += (wheel_revs_per_sec * dt);
            if (wheel_fractional >= 1.0f) {
                uint32_t whole_revs = (uint32_t)wheel_fractional;
                ble_cumulative_wheel_revs += whole_revs;
                wheel_fractional -= whole_revs;
                ble_last_wheel_event_time = (uint16_t)((esp_timer_get_time() * 1024) / 1000000);
            }
        }

        if (s.cadence > 0) {
            float crank_revs_per_sec = s.cadence / 60.0f;
            static float crank_fractional = 0.0f;
            crank_fractional += (crank_revs_per_sec * dt);
            if (crank_fractional >= 1.0f) {
                uint16_t whole_cranks = (uint16_t)crank_fractional;
                ble_cumulative_crank_revs += whole_cranks;
                crank_fractional -= whole_cranks;
                ble_last_crank_event_time = (uint16_t)((esp_timer_get_time() * 1024) / 1000000);
            }
        }
    }


    CTS:

    typedef struct __attribute__((packed)) {
        uint16_t motor_power_watts;  // Instantaneous motor power consumption
        uint16_t estimated_range_km; // Range remaining (16-bit, 0.1 km resolution)
        uint8_t  assist_level;       // Current assist mode (0 = Off, 1 = Eco ... 5 = Boost)
        uint8_t  error_code;         // Current Bafang system error code (0 = Normal)
        uint16_t battery_voltage_mv; // Raw battery pack voltage in millivolts
        int16_t  motor_temp_c;       // Controller/Motor temperature in °C
    } ebike_telemetry_t;



    NUS:

    ┌──────────────┬──────────────┬──────────────────┬──────────────────────┐
    │  Type (ID)   │ Length (Len) │ Value (Payload)  │   CRC-16 (Optional)  │
    │    1 Byte    │   2 Bytes    │    Len Bytes     │        2 Bytes       │
    └──────────────┴──────────────┴──────────────────┴──────────────────────┘

    Example future-proof types:
    enum class NusMsgType : uint8_t {
        // 0x00 - 0x1F: Core Vehicle Operations
        SET_PAS_LEVEL        = 0x01,  // Payload: int8_t
        SET_BIKE_LOCK        = 0x02,  // Payload: uint8_t (0=Unlock, 1=Lock)

        // 0x20 - 0x3F: Navigation Engine (e.g., OsmAnd integration)
        NAV_TURN_INSTRUCTION = 0x20,  // Payload: Struct { uint8_t icon_id, uint32_t distance_m, char street[32] }
        NAV_ETA_UPDATE       = 0x21,  // Payload: uint32_t (unix timestamp)

        // 0x40 - 0x5F: BLE Client / HRM Manager Bridge
        HRM_START_SCAN       = 0x40,  // Payload: none
        HRM_SCAN_RESULT      = 0x41,  // Payload: Struct { uint8_t mac[6], int8_t rssi, char name[16] }
        HRM_CONNECT          = 0x42,  // Payload: uint8_t mac[6]
        HRM_STATUS           = 0x43,  // Payload: uint8_t (0=Disconnected, 1=Scanning, 2=Connected)

        // 0x60 - 0x7F: Display Settings & Customization
        SET_UI_THEME         = 0x60,  // Payload: uint8_t
        SET_WHEEL_CIRC       = 0x61   // Payload: uint16_t (mm)
    };

    Api class will need to be updated to handle these messages. MTU negotiation and fragmentation will be handled by the BLE task.
    nusRX::onWrite(msg) { api.queueNusMsg(msg); }
    Api::handleNusMsg(msg) { process(msg); nusTX.write(processedMsg); }



*/