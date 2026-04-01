# Phase 2: BLE Connectivity

## Objective

Introduce Bluetooth Low Energy (BLE) communication for telemetry streaming and external interaction while maintaining system stability and real-time performance.

## Success Criteria

- ✅ BLE server advertising and connectable
- ✅ Mobile app can discover and connect to device
- ✅ Telemetry data streamed via BLE (1-10Hz)
- ✅ Configuration parameters accessible via BLE
- ✅ System remains stable with BLE active
- ✅ Power consumption within acceptable limits

## Scope

### BLE Server Implementation
- Custom BLE service with API characteristics
- Standard Bluetooth SIG services (cycling, battery, device info)
- Connection management and security
- Data streaming with configurable update rates

### System Integration
- Extend data model to support BLE publication
- Implement BLE update throttling to conserve bandwidth
- Ensure non-blocking interaction with existing tasks
- Add BLE state to system status display

### Optional Hardware
- Ambient temperature sensor (DS18B20)
- Light sensor for automatic backlight control
- Accelerometer for motion-based features

## BLE Architecture

### Service Structure
```
OpenRideDash Device (Peripheral)
├── Device Information Service (0x180A)
│   ├── Manufacturer Name (0x2A29)
│   ├── Model Number (0x2A24)
│   └── Firmware Revision (0x2A26)
├── Battery Service (0x180F)
│   └── Battery Level (0x2A19)
├── Cycling Power Service (0x1818) [Optional]
│   ├── Cycling Power Measurement (0x2A63)
│   └── Cycling Power Feature (0x2A65)
└── OpenRideDash API Service (Custom UUID)
    ├── API Info (Read)
    ├── Command RX (Write)
    └── Event TX (Notify)
```

### Connection Flow
```
1. Device advertises with name "OpenRideDash-XXXX"
2. Mobile app scans and discovers device
3. App connects and pairs (if security enabled)
4. App reads API Info to determine capabilities
5. App subscribes to Event TX for notifications
6. Bidirectional communication established
```

## Implementation Details

### BLE Task Implementation
```cpp
// tasks/ble_task.cpp
class BleTask : public BaseTask {
public:
    void setup() override {
        // Initialize BLE stack
        BLEDevice::init("OpenRideDash");
        
        // Create server
        server = BLEDevice::createServer();
        server->setCallbacks(this);
        
        // Create services
        createDeviceInfoService();
        createBatteryService();
        createApiService();
        
        // Start advertising
        startAdvertising();
    }
    
    void run() override {
        while (true) {
            // Handle BLE events
            processBleEvents();
            
            // Update connected clients
            updateSubscribers();
            
            // Handle power management
            managePowerState();
            
            vTaskDelay(pdMS_TO_TICKS(100)); // 10Hz
        }
    }
    
private:
    BLEServer* server;
    BLEAdvertising* advertising;
    
    void createApiService() {
        BLEService* apiService = server->createService(API_SERVICE_UUID);
        
        // API Info characteristic (read only)
        BLECharacteristic* infoChar = apiService->createCharacteristic(
            API_INFO_UUID,
            BLECharacteristic::PROPERTY_READ
        );
        infoChar->setValue(apiInfoData, sizeof(apiInfoData));
        
        // Command RX characteristic (write only)
        BLECharacteristic* cmdChar = apiService->createCharacteristic(
            COMMAND_RX_UUID,
            BLECharacteristic::PROPERTY_WRITE
        );
        cmdChar->setCallbacks(this);
        
        // Event TX characteristic (notify)
        BLECharacteristic* eventChar = apiService->createCharacteristic(
            EVENT_TX_UUID,
            BLECharacteristic::PROPERTY_NOTIFY
        );
        
        apiService->start();
    }
};
```

### API Protocol Implementation
```cpp
// services/ble_api.cpp
class BleApiService {
public:
    void onWrite(BLECharacteristic* characteristic) {
        std::string value = characteristic->getValue();
        
        if (value.length() < 3) return; // Minimum packet size
        
        uint8_t type = value[0];
        uint8_t id = value[1];
        uint8_t len = value[2];
        
        switch (type) {
            case 0x01: // Command
                handleCommand(id, value.substr(3, len));
                break;
                
            case 0x02: // Response (should not be received)
                logError("Unexpected response from client");
                break;
        }
    }
    
    void sendEvent(uint8_t eventId, const uint8_t* data, size_t length) {
        if (!isConnected()) return;
        
        uint8_t packet[20];
        packet[0] = 0x03; // Event type
        packet[1] = eventId;
        packet[2] = length;
        
        if (length > 0) {
            memcpy(&packet[3], data, length);
        }
        
        eventCharacteristic->setValue(packet, length + 3);
        eventCharacteristic->notify();
    }
};
```

### Data Streaming
```cpp
// services/telemetry_stream.cpp
class TelemetryStream {
public:
    void update() {
        if (!shouldSendUpdate()) return;
        
        TelemetryPacket packet;
        packet.timestamp = millis();
        packet.batteryVoltage = systemState.batteryVoltage;
        packet.batteryCurrent = systemState.batteryCurrent;
        packet.speed = systemState.speed;
        packet.pasLevel = systemState.pasLevel;
        
        bleApi.sendEvent(EVENT_TELEMETRY, 
                        reinterpret_cast<uint8_t*>(&packet),
                        sizeof(packet));
        
        lastUpdate = packet.timestamp;
    }
    
private:
    bool shouldSendUpdate() {
        uint32_t now = millis();
        uint32_t interval = getUpdateInterval(); // Configurable 100-1000ms
        
        return (now - lastUpdate) >= interval && isConnected();
    }
    
    uint32_t lastUpdate = 0;
};
```

## Standard BLE Services

### Device Information Service
```cpp
void createDeviceInfoService() {
    BLEService* devInfo = server->createService(BLEUUID((uint16_t)0x180A));
    
    // Manufacturer
    BLECharacteristic* manufChar = devInfo->createCharacteristic(
        BLEUUID((uint16_t)0x2A29),
        BLECharacteristic::PROPERTY_READ
    );
    manufChar->setValue("OpenRideDash");
    
    // Model Number
    BLECharacteristic* modelChar = devInfo->createCharacteristic(
        BLEUUID((uint16_t)0x2A24),
        BLECharacteristic::PROPERTY_READ
    );
    modelChar->setValue("ORD-C3-1.0");
    
    // Firmware Revision
    BLECharacteristic* fwChar = devInfo->createCharacteristic(
        BLEUUID((uint16_t)0x2A26),
        BLECharacteristic::PROPERTY_READ
    );
    fwChar->setValue("1.0.0");
    
    devInfo->start();
}
```

### Battery Service
```cpp
void updateBatteryLevel() {
    if (!batteryCharacteristic) return;
    
    uint8_t level = calculateBatteryPercent();
    batteryCharacteristic->setValue(&level, 1);
    
    // Only notify if subscribed
    if (batteryCharacteristic->getSubscribedCount() > 0) {
        batteryCharacteristic->notify();
    }
}
```

## Optional Sensor Integration

### Temperature Sensor
```cpp
// drivers/ds18b20.cpp
class DS18B20Sensor : public TempSensor {
public:
    float read() override {
        if (!sensor.isConnected()) return NAN;
        
        sensor.requestTemperatures();
        float temp = sensor.getTempC();
        
        // Apply calibration if needed
        temp += calibrationOffset;
        
        return temp;
    }
    
private:
    DallasTemperature sensor;
    float calibrationOffset = 0.0f;
};
```

### Light Sensor Integration
```cpp
// services/backlight_control.cpp
class BacklightController {
public:
    void update() {
        if (!lightSensor) return;
        
        float lux = lightSensor->read();
        
        // Adaptive backlight control
        if (lux < 50) {        // Dark
            display.setBacklight(100);
        } else if (lux < 1000) { // Indoor
            display.setBacklight(70);
        } else {                // Bright sunlight
            display.setBacklight(30); // Save power
        }
    }
};
```

## Power Management

### BLE Power States
```cpp
enum class BlePowerState {
    ADVERTISING,      // Low power, discoverable
    CONNECTED,        // Active communication
    CONNECTED_IDLE,   // Connected but idle
    DISCONNECTED      // Not advertising
};

void managePowerState() {
    switch (currentPowerState) {
        case BlePowerState::ADVERTISING:
            if (isConnected()) {
                setPowerState(BlePowerState::CONNECTED);
            }
            break;
            
        case BlePowerState::CONNECTED:
            if (!isConnected()) {
                setPowerState(BlePowerState::ADVERTISING);
            } else if (isIdle()) {
                setPowerState(BlePowerState::CONNECTED_IDLE);
            }
            break;
            
        case BlePowerState::CONNECTED_IDLE:
            if (!isConnected()) {
                setPowerState(BlePowerState::ADVERTISING);
            } else if (!isIdle()) {
                setPowerState(BlePowerState::CONNECTED);
            }
            break;
    }
}
```

### Connection Parameters
```cpp
void setConnectionParameters() {
    // Fast connection for quick data transfer
    BLEConnectionParameters params;
    params.setInterval(6, 12);      // 7.5-15ms interval
    params.setLatency(0);           // No latency
    params.setTimeout(400);         // 4 second timeout
    
    for (auto& conn : server->getConnections()) {
        conn->updateConnectionParams(&params);
    }
}
```

## Security Implementation

### Pairing and Bonding
```cpp
class SecurityManager {
public:
    void setup() {
        BLESecurity* security = new BLESecurity();
        
        // Set security levels
        security->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);
        security->setCapability(ESP_IO_CAP_OUT);
        security->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
        
        // Set static passkey (configurable via app)
        security->setStaticPin(passkey);
    }
    
    void setPasskey(uint32_t newPasskey) {
        passkey = newPasskey;
        saveToConfig();
    }
    
private:
    uint32_t passkey = 123456; // Default
};
```

## Development Tasks Checklist

### BLE Foundation
- [ ] Initialize BLE stack with device name
- [ ] Create BLE server with connection callbacks
- [ ] Implement standard services (device info, battery)
- [ ] Create custom API service with characteristics
- [ ] Implement advertising with scan response

### API Protocol
- [ ] Define packet format (type, id, length, payload)
- [ ] Implement command handling system
- [ ] Create event notification system
- [ ] Add error handling and validation
- [ ] Implement version negotiation

### Data Streaming
- [ ] Extend data model for BLE publication
- [ ] Implement telemetry streaming service
- [ ] Add configurable update rates (1Hz, 5Hz, 10Hz)
- [ ] Implement data throttling and batching
- [ ] Add connection quality monitoring

### System Integration
- [ ] Create BLE task with proper priority
- [ ] Integrate with existing CAN and display tasks
- [ ] Implement thread-safe data sharing
- [ ] Add BLE status to display
- [ ] Handle BLE events in non-blocking manner

### Optional Features
- [ ] Add temperature sensor support
- [ ] Implement automatic backlight control
- [ ] Add motion detection with accelerometer
- [ ] Implement power management for BLE

### Testing
- [ ] Test connection/disconnection cycles
- [ ] Verify data streaming accuracy
- [ ] Test with multiple mobile platforms
- [ ] Validate power consumption
- [ ] Stress test with high data rates

## Testing Procedures

### BLE Functionality Tests
1. **Discovery Test**: Verify device appears in BLE scans
2. **Connection Test**: Establish and maintain connection
3. **Data Integrity Test**: Verify telemetry data accuracy
4. **Multiple Client Test**: Handle multiple connection attempts
5. **Range Test**: Verify operation at typical distances (10m)

### Performance Tests
1. **Update Rate Test**: Verify configurable update rates
2. **Latency Test**: Measure command response times
3. **Power Test**: Measure current draw in different states
4. **Memory Test**: Monitor heap usage during operation

### Integration Tests
1. **CAN + BLE Test**: Ensure both subsystems work concurrently
2. **Display + BLE Test**: Verify display updates during BLE activity
3. **Keypad + BLE Test**: Test user input during BLE operations
4. **Stress Test**: High message rates from both CAN and BLE

## Troubleshooting

### Common BLE Issues

#### Connection Problems
- **Device not discoverable**: Check advertising is active, verify device name
- **Connection fails**: Check pairing requirements, verify passkey
- **Drops connection**: Check signal strength, adjust connection parameters

#### Data Issues
- **Missing notifications**: Verify client has subscribed, check MTU size
- **Corrupted data**: Verify packet format, check buffer sizes
- **Slow updates**: Adjust connection interval, reduce payload size

#### Performance Issues
- **High latency**: Reduce connection interval, minimize processing delays
- **High power consumption**: Use longer connection intervals, optimize advertising
- **Memory leaks**: Monitor heap usage, ensure proper cleanup

### Debugging Tools
1. **nRF Connect**: BLE debugging and packet analysis
2. **BLE Scanner Apps**: For testing discovery and connection
3. **Serial Monitor**: System logs and BLE stack debug output
4. **Power Profiler**: Measure current consumption during BLE operations

## Performance Targets

### Connection Parameters
- **Advertising Interval**: 100-1000ms (configurable)
- **Connection Interval**: 7.5-15ms for active, 100-500ms for idle
- **MTU Size**: 247 bytes (ESP32 maximum)
- **Data Rate**: Up to 1.4Mbps theoretical, ~100kbps practical

### Resource Usage
- **BLE Stack Memory**: ~50KB RAM
- **Connection Memory**: ~1KB per connection
- **CPU Usage**: < 10% average during BLE operations
- **Power Consumption**: < 20mA additional when connected

### Reliability Targets
- **Connection Success Rate**: > 99%
- **Data Delivery Rate**: > 99.9%
- **Reconnection Time**: < 2 seconds
- **Uptime**: 24+ hours continuous operation

## Configuration Options

### BLE Settings
```cpp
struct BleConfig {
    char deviceName[32];      // "OpenRideDash-XXXX"
    uint16_t advInterval;     // Advertising interval (ms)
    uint16_t connIntervalMin; // Minimum connection interval (ms)
    uint16_t connIntervalMax; // Maximum connection interval (ms)
    uint16_t slaveLatency;    // Slave latency
    uint16_t connTimeout;     // Connection timeout (ms)
    uint8_t txPower;          // TX power level (dBm)
    bool usePairing;          // Enable pairing security
    uint32_t passkey;         // Pairing passkey
};
```

### Telemetry Settings
```cpp
struct TelemetryConfig {
    uint16_t updateInterval;  // Milliseconds between updates
    bool streamBattery;       // Stream battery data
    bool streamSpeed;         // Stream speed data
    bool streamPower;         // Stream power data
    bool streamTemperature;   // Stream temperature data
    uint8_t batchSize;        // Number of samples per packet
};
```

## Next Steps

After completing Phase 2 successfully, proceed to:

1. **[Phase 3: Mobile App](../phases/phase3-mobile.md)** - Develop companion application
2. **[Advanced Features](../phases/phase4-advanced.md)** - OTA updates and ecosystem integration
3. **[Hardware Enclosure](../hardware/assembly.md)** - Finalize weatherproof design

## Additional Resources

- **[BLE API Documentation](../api/ble-api.md)** - Complete protocol specification
- **[Development Guide](../development/guide.md)** - BLE development setup
- **[Hardware Integration](../hardware/guide.md)** - Sensor wiring and testing
- **[Performance Tuning](../development/optimization.md)** - BLE optimization techniques

---

*Phase 2 enables wireless connectivity, opening up possibilities for mobile integration, remote configuration, and expanded sensor networks while maintaining the reliability established in Phase 1.*