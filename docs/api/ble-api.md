# BLE API Documentation

## Overview

OpenRideDash exposes a Bluetooth Low Energy (BLE) API for communication with mobile applications and external tools. This document describes the BLE services, characteristics, commands, and data formats used by the system.

## Service Architecture

### Service UUIDs

| Service                       | UUID                                   | Description                                          |
| ----------------------------- | -------------------------------------- | ---------------------------------------------------- |
| Device Information            | `0x180A`                               | Standard device information service                  |
| Battery Service               | `0x180F`                               | Battery level monitoring                             |
| Cycling Power Service         | `0x1818`                               | Cycling power and speed data (optional)              |
| OpenRideDash Custom Telemetry | `C3A9xxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx` | Custom telemetry fields for controller-specific data |
| OpenRideDash API              | `C3A8xxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx` | Custom API for configuration and control             |

### Custom Service UUID

The OpenRideDash API service uses a custom UUID base: `C3A80000-xxxx-xxxx-xxxx-xxxxxxxxxxxx`. Specific characteristic UUIDs are derived from this base.

## Device Information Service

### Characteristics

#### Manufacturer Name String

- **UUID**: `0x2A29`
- **Properties**: Read
- **Format**: UTF-8 string
- **Value**: "OpenRideDash"

#### Model Number String

- **UUID**: `0x2A24`
- **Properties**: Read
- **Format**: UTF-8 string
- **Value**: "ORD-C3-1.0"

#### Firmware Revision String

- **UUID**: `0x2A26`
- **Properties**: Read
- **Format**: UTF-8 string
- **Value**: e.g., "1.2.3"

#### Hardware Revision String

- **UUID**: `0x2A27`
- **Properties**: Read
- **Format**: UTF-8 string
- **Value**: e.g., "RevB"

#### Serial Number String

- **UUID**: `0x2A25`
- **Properties**: Read
- **Format**: UTF-8 string
- **Value**: Device unique serial number

## Battery Service

### Battery Level

- **UUID**: `0x2A19`
- **Properties**: Read, Notify
- **Format**: uint8 (0-100%)
- **Update Frequency**: 1Hz when connected, on change otherwise

## Cycling Power Service (Optional)

### Cycling Power Measurement

- **UUID**: `0x2A63`
- **Properties**: Notify
- **Format**: Bluetooth SIG Cycling Power Measurement structure
- **Update Frequency**: Configurable (1-10Hz)

### Cycling Power Feature

- **UUID**: `0x2A65`
- **Properties**: Read
- **Format**: 32-bit feature bitmap
- **Features Supported**: Wheel revolutions, Crank revolutions, Power balance

## OpenRideDash Custom Telemetry Service

### Service UUID

- **Base**: `C3A90000-xxxx-xxxx-xxxx-xxxxxxxxxxxx`
- **Full Service UUID**: `C3A90000-1234-5678-9ABC-DEF012345678`

### Purpose

This service provides flexible telemetry characteristics for controller-specific data fields. Different e-bike controllers provide different telemetry fields (motor power, motor temperature, etc.), while others may not provide these fields at all. The custom telemetry service allows the system to expose only the characteristics that are available from the connected controller, providing adaptability to different hardware capabilities.

### Characteristics

#### Motor Power (Notify)

- **UUID**: `C3A90001-1234-5678-9ABC-DEF012345678`
- **Properties**: Notify
- **Format**: int16_t (watts)
- **Update Frequency**: Configurable (1-10Hz)
- **Description**: Motor power output in watts. Only available if supported by the connected controller.

#### Motor Temperature (Notify)

- **UUID**: `C3A90002-1234-5678-9ABC-DEF012345678`
- **Properties**: Notify
- **Format**: int16_t (degrees Celsius × 10)
- **Update Frequency**: 1Hz
- **Description**: Motor temperature in tenths of a degree Celsius. Only available if supported by the connected controller.

#### Controller Voltage (Notify)

- **UUID**: `C3A90003-1234-5678-9ABC-DEF012345678`
- **Properties**: Notify
- **Format**: uint16_t (millivolts)
- **Update Frequency**: 1Hz
- **Description**: Controller input voltage in millivolts. Only available if supported by the connected controller.

#### Custom Telemetry Field (Notify)

- **UUID**: `C3A90004-1234-5678-9ABC-DEF012345678`
- **Properties**: Notify
- **Format**: Variable (depends on field)
- **Update Frequency**: Configurable
- **Description**: Generic characteristic for other controller-specific telemetry fields. The format and meaning are determined by the controller type.

## OpenRideDash API Service

### Service UUID

- **Base**: `C3A80000-xxxx-xxxx-xxxx-xxxxxxxxxxxx`
- **Full Service UUID**: `C3A80000-1234-5678-9ABC-DEF012345678`

### Characteristics

#### API Info (Read Only)

- **UUID**: `C3A80001-1234-5678-9ABC-DEF012345678`
- **Properties**: Read
- **Format**: 20-byte structure

```
Offset  Size  Field          Description
0       2     Protocol Version  (e.g., 0x0100 for v1.0)
2       2     API Version       (e.g., 0x0100 for v1.0)
4       4     Firmware Version  (32-bit integer)
8       4     Hardware Version  (32-bit integer)
12      4     Capabilities Bitmask
16      4     Max Packet Size   (bytes)
```

#### Command RX (Write Only)

- **UUID**: `C3A80002-1234-5678-9ABC-DEF012345678`
- **Properties**: Write, Write Without Response
- **Format**: 20-byte packet (TYPE, ID, LEN, PAYLOAD)
- **Description**: Commands sent from client to device. Packets are restricted to 20 bytes total.

#### Event TX (Notify)

- **UUID**: `C3A80003-1234-5678-9ABC-DEF012345678`
- **Properties**: Notify
- **Format**: 20-byte packet (TYPE, ID, LEN, PAYLOAD)
- **Description**: Command responses and system events sent from device to client. Not used for telemetry data.

#### Configuration (Read/Write)

- **UUID**: `C3A80004-1234-5678-9ABC-DEF012345678`
- **Properties**: Read, Write
- **Format**: JSON or binary configuration structure
- **Description**: Device configuration parameters

## Command Protocol

### Packet Format

All commands and events use the same basic packet structure, restricted to 20 bytes total length:

```
[TYPE][ID][LEN][PAYLOAD...]
```

**Byte layout:**

| Offset | Size (bytes) | Field   | Description                        |
| ------ | ------------ | ------- | ---------------------------------- |
| 0      | 1            | TYPE    | Packet type (0x01-0x03)            |
| 1      | 1            | ID      | Command/response/event identifier  |
| 2      | 1            | LEN     | Payload length (0–17)              |
| 3      | LEN          | PAYLOAD | Message-specific data (0-17 bytes) |

**Total packet size:** 3 + LEN bytes (maximum 20 bytes)

**TYPE values:**

- `0x01` = Command
- `0x02` = Response
- `0x03` = Event

**ID:** Command/response/event identifier (specific values defined per command set)

**LEN:** Number of bytes in PAYLOAD (0–17). The total packet length must not exceed 20 bytes.

**PAYLOAD:** Variable-length data whose format depends on the command/event ID.

### Command Types

#### System Commands (0x01-0x0F)

| ID   | Name          | Payload         | Description                   |
| ---- | ------------- | --------------- | ----------------------------- |
| 0x01 | GetSystemInfo | None            | Get system information        |
| 0x02 | GetStatus     | None            | Get current system status     |
| 0x03 | Reset         | uint8 (type)    | Reset system (0=soft, 1=hard) |
| 0x04 | EnterOTA      | None            | Enter OTA update mode         |
| 0x05 | GetConfig     | uint8 (section) | Get configuration             |
| 0x06 | SetConfig     | Variable        | Set configuration             |
| 0x07 | SaveConfig    | None            | Save configuration to flash   |
| 0x08 | FactoryReset  | None            | Reset to factory defaults     |

#### CAN Commands (0x10-0x1F)

| ID   | Name         | Payload       | Description                            |
| ---- | ------------ | ------------- | -------------------------------------- |
| 0x10 | SendCAN      | CAN frame     | Send CAN message                       |
| 0x11 | SetCANFilter | Filter config | Set CAN filter                         |
| 0x12 | GetCANStats  | None          | Get CAN statistics                     |
| 0x13 | SetCANMode   | uint8 (mode)  | Set CAN mode (0=normal, 1=listen only) |

#### Display Commands (0x20-0x2F)

| ID   | Name           | Payload       | Description                    |
| ---- | -------------- | ------------- | ------------------------------ |
| 0x20 | SetDisplayPage | uint8 (page)  | Set display page               |
| 0x21 | SetBrightness  | uint8 (level) | Set display brightness (0-100) |
| 0x22 | SetContrast    | uint8 (level) | Set display contrast (0-100)   |
| 0x23 | DisplayText    | Variable      | Display custom text            |

#### BLE Commands (0x30-0x3F)

| ID   | Name           | Payload     | Description                 |
| ---- | -------------- | ----------- | --------------------------- |
| 0x30 | SetBLEInterval | uint16 (ms) | Set BLE connection interval |
| 0x31 | SetBLETXPower  | int8 (dBm)  | Set BLE transmit power      |
| 0x32 | GetBLEDevices  | None        | Get paired BLE devices      |
| 0x33 | ConnectBLE     | MAC address | Connect to BLE sensor       |

#### Sensor Commands (0x40-0x4F)

| ID   | Name              | Payload     | Description                  |
| ---- | ----------------- | ----------- | ---------------------------- |
| 0x40 | GetTemperature    | None        | Get temperature reading      |
| 0x41 | CalibrateSensor   | Variable    | Calibrate sensor             |
| 0x42 | SetSensorInterval | uint16 (ms) | Set sensor sampling interval |

### Response Format

Responses use Type = 0x02 with the same ID as the command:

```
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   0x02        |   ID (1)      |   Length (1)  |   Status (1) |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                         Data (0-240 bytes)                   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

**Status Codes:**

- `0x00`: Success
- `0x01`: Invalid command
- `0x02`: Invalid parameter
- `0x03`: Operation failed
- `0x04`: Not supported
- `0x05`: Busy
- `0x06`: Timeout

### Event Format

Events use Type = 0x03:

```
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   0x03        |   Event ID    |   Length (1)  |   Reserved   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                         Data (0-240 bytes)                   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

#### Event Types

| Event ID | Name            | Data                         | Description             |
| -------- | --------------- | ---------------------------- | ----------------------- |
| 0x02     | ButtonPress     | uint8 (button), uint8 (type) | Button press event      |
| 0x03     | CANMessage      | CAN frame                    | CAN message received    |
| 0x04     | BLEConnected    | MAC address                  | BLE device connected    |
| 0x05     | BLEDisconnected | MAC address                  | BLE device disconnected |
| 0x06     | ErrorOccurred   | Error struct                 | System error occurred   |
| 0x07     | ConfigChanged   | Config section               | Configuration changed   |
| 0x08     | LowBattery      | uint8 (percent)              | Low battery warning     |
| 0x09     | OTAProgress     | uint8 (percent)              | OTA update progress     |

## Configuration Structure

### Configuration Sections

#### System Configuration (Section 0x01)

```
struct SystemConfig {
    uint8_t displayBrightness;   // 0-100
    uint8_t displayContrast;     // 0-100
    uint16_t sleepTimeout;       // seconds (0 = disabled)
    uint8_t backlightMode;       // 0=auto, 1=manual, 2=always on
    uint8_t units;              // 0=metric, 1=imperial
} __attribute__((packed));
```

#### CAN Configuration (Section 0x02)

```
struct CanConfig {
    uint32_t baudrate;           // 125000, 250000, 500000, 1000000
    uint8_t mode;               // 0=normal, 1=listen only, 2=loopback
    CanFilter filters[4];        // CAN filters
    uint8_t filterCount;         // Number of active filters
} __attribute__((packed));
```

#### BLE Configuration (Section 0x03)

```
struct BleConfig {
    char deviceName[32];         // "OpenRideDash-XXXX"
    uint16_t advInterval;        // advertising interval (ms)
    uint16_t connIntervalMin;    // connection interval min (ms)
    uint16_t connIntervalMax;    // connection interval max (ms)
    uint16_t slaveLatency;       // slave latency
    uint16_t connTimeout;        // connection timeout (ms)
    uint8_t txPower;            // TX power level (dBm)
    bool usePairing;            // enable pairing security
    uint32_t passkey;           // pairing passkey
} __attribute__((packed));
```

#### Telemetry Configuration (Section 0x04)

```
struct TelemetryConfig {
    uint16_t updateInterval;     // milliseconds between updates
    bool streamBattery;          // stream battery data
    bool streamSpeed;            // stream speed data
    bool streamPower;            // stream power data
    bool streamTemperature;      // stream temperature data
    uint8_t batchSize;           // number of samples per packet
} __attribute__((packed));
```

## Error Codes

### System Errors (0x0000-0x00FF)

| Code   | Name             | Description            |
| ------ | ---------------- | ---------------------- |
| 0x0000 | NoError          | No error               |
| 0x0001 | InvalidCommand   | Unknown command ID     |
| 0x0002 | InvalidParameter | Parameter out of range |
| 0x0003 | OperationFailed  | Operation failed       |
| 0x0004 | NotSupported     | Feature not supported  |
| 0x0005 | Busy             | System busy            |
| 0x0006 | Timeout          | Operation timeout      |
| 0x0007 | MemoryFull       | Memory full            |
| 0x0008 | ChecksumError    | Checksum error         |

### CAN Errors (0x0100-0x01FF)

| Code   | Name               | Description               |
| ------ | ------------------ | ------------------------- |
| 0x0100 | CANBusOff          | CAN bus off error         |
| 0x0101 | CANWarning         | CAN warning level reached |
| 0x0102 | CANPassive         | CAN error passive         |
| 0x0103 | CANArbitrationLost | Arbitration lost          |
| 0x0104 | CANBusError        | Bus error detected        |

### BLE Errors (0x0200-0x02FF)

| Code   | Name                      | Description              |
| ------ | ------------------------- | ------------------------ |
| 0x0200 | BLENotInitialized         | BLE not initialized      |
| 0x0201 | BLENotConnected           | No BLE connection        |
| 0x0202 | BLECharacteristicNotFound | Characteristic not found |
| 0x0203 | BLEWriteFailed            | Write operation failed   |
| 0x0204 | BLEReadFailed             | Read operation failed    |

## Security Considerations

### Pairing and Bonding

- **Just Works**: Default pairing method for simplicity
- **Passkey Entry**: Optional for increased security
- **Bonding**: Device stores bonding information in secure storage
- **Encryption**: All communication encrypted when paired

### Access Control

- **Read/Write Permissions**: Configured per characteristic
- **Authentication**: Required for sensitive operations
- **Authorization**: Some commands require specific authorization level

### Firmware Validation

- **Signature Verification**: OTA updates verified with cryptographic signatures
- **Rollback Protection**: Prevents downgrade to vulnerable versions
- **Secure Boot**: Optional secure boot implementation

## Implementation Notes

### MTU Considerations

- **Default MTU**: 23 bytes (BLE 4.0 minimum)
- **Negotiated MTU**: Up to 247 bytes with BLE 4.2+
- **Packet Size**: Maximum payload = MTU - 3 bytes (ATT header)

### Connection Parameters

- **Interval**: 7.5ms - 4s (configurable)
- **Latency**: 0-499 (configurable)
- **Timeout**: 500ms - 32s (configurable)
- **Recommended**: 15-30ms interval, 0 latency, 2s timeout for real-time data

### Power Optimization

- **Advertising Interval**: 100-1000ms (configurable)
- **Connection Interval**: Balance between latency and power
- **Slave Latency**: Skip connection events when no data
- **TX Power**: Adjust based on required range

## Example Usage

### Reading Device Information

```python
import asyncio
from bleak import BleakClient

async def get_device_info():
    async with BleakClient(device_address) as client:
        # Read manufacturer name
        manufacturer = await client.read_gatt_char("00002a29-0000-1000-8000-00805f9b34fb")
        print(f"Manufacturer: {manufacturer.decode('utf-8')}")

        # Read firmware version
        fw_rev = await client.read_gatt_char("00002a26-0000-1000-8000-00805f9b34fb")
        print(f"Firmware: {fw_rev.decode('utf-8')}")
```

### Subscribing to Telemetry

Telemetry data is available through multiple characteristics in the OpenRideDash Custom Telemetry Service. Each characteristic provides a specific telemetry field. Here's an example of subscribing to motor power updates:

```python
MOTOR_POWER_UUID = "C3A90001-1234-5678-9ABC-DEF012345678"

def motor_power_callback(sender, data):
    # Data is int16_t (watts)
    power = int.from_bytes(data, byteorder='little', signed=True)
    print(f"Motor power: {power}W")

async def subscribe_motor_power():
    async with BleakClient(device_address) as client:
        await client.start_notify(MOTOR_POWER_UUID, motor_power_callback)
        await asyncio.sleep(30)  # Listen for 30 seconds
        await client.stop_notify(MOTOR_POWER_UUID)
```

**Note:** Not all telemetry characteristics may be available on every device. The available characteristics depend on the capabilities of the connected e-bike controller. Before subscribing, check if the characteristic exists by reading the device's capabilities or attempting to discover services.

### Sending Commands

```python
async def set_brightness(level):
    async with BleakClient(device_address) as client:
        command = bytes([0x01, 0x21, 0x01, 0x00, level])  # SetBrightness command
        await client.write_gatt_char(COMMAND_CHAR_UUID, command, response=True)
```

## References

- [Bluetooth SIG Assigned Numbers](https://www.bluetooth.com/specifications/assigned-numbers/)
- [BLE GATT Specification](https://www.bluetooth.com/specifications/gatt/)
