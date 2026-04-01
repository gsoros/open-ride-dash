# OpenRideDash (ORD) — BLE API Specification (Condensed)

## API Service Architecture

### Custom BLE Service: ORD API

The ORD API is implemented as a custom BLE service with three characteristics:

#### 1. API Info (Read)

Provides protocol metadata and device capabilities.

**Format:**

```
[protocol_major][protocol_minor][capabilities...]
```

* `protocol_major` (1 byte): breaking changes
* `protocol_minor` (1 byte): backward-compatible changes
* `capabilities` (N bytes, bitmask): feature flags (e.g. OTA, HRM, config write)

**Notes:**

* Must be read by the client immediately after connection
* Defines compatibility before any commands are sent

---

#### 2. Command RX (Write)

Used by the client (phone app) to send commands to the device.

* Write with response recommended
* One packet per command

---

#### 3. Event TX (Notify)

Used by the device to send:

* command responses
* asynchronous events

---

## Connection Flow

1. Client connects
2. Client reads **API Info**
3. Client verifies protocol compatibility
4. Client begins command/response communication

Optional fallback:

* Client may issue `GET_INFO` command if needed

---

## Packet Format (20-byte safe)

All messages (commands, responses, events) use the same structure:

```
[TYPE][ID][LEN][PAYLOAD...]
```

* `TYPE` (1 byte):

  * `0x01` = Command
  * `0x02` = Response
  * `0x03` = Event

* `ID` (1 byte):

  * Command / response / event identifier

* `LEN` (1 byte):

  * Payload length (0–17 bytes)

* `PAYLOAD` (0–17 bytes):

  * Message-specific data

**Total max size: 20 bytes**

---

## Core Commands

### GET_INFO (0x01)

Request protocol version and capabilities.

**Request:**

```
[0x01][0x01][0x00]
```

**Response:**

```
[0x02][0x01][N][protocol_major][protocol_minor][capabilities...]
```

---

### GET_PARAM (0x02)

Request value of a parameter.

**Request:**

```
[0x01][0x02][1][param_id]
```

**Response:**

```
[0x02][0x02][N][param_id][value...]
```

---

### SET_PARAM (0x03)

Set value of a parameter.

**Request:**

```
[0x01][0x03][N][param_id][value...]
```

**Response:**

```
[0x02][0x03][1][status]
```

* `status`: 0x00 = OK, non-zero = error

---

### ACTION (0x04)

Trigger an action (e.g. sleep, OTA enable).

**Request:**

```
[0x01][0x04][N][action_id][params...]
```

**Response:**

```
[0x02][0x04][1][status]
```

---

## Events

### PARAM_VALUE (0x10)

Asynchronous parameter update.

```
[0x03][0x10][N][param_id][value...]
```

---

### INPUT_EVENT (0x11)

Keypad or user input event.

```
[0x03][0x11][N][event_id][data...]
```

---

### ERROR (0x7F)

Indicates invalid command or failure.

```
[0x03][0x7F][N][error_code][info...]
```

---

## Design Notes

* No CRC: BLE link layer already ensures reliability
* No fragmentation: all messages fit within 20 bytes
* One message per operation for simplicity and robustness
* Versioning handled via API Info characteristic
* Capabilities bitmask prevents hardcoding feature assumptions
* Unknown parameters or commands must be safely ignored

---

## Versioning Rules

* Major version mismatch → client must not proceed
* Minor version mismatch → client may proceed, ignoring unknown fields

---

## Future Extensions

* Larger MTU may be used to increase payload size (optional optimization)
* Bulk configuration transfer may be added later if needed
* Additional commands and events can be introduced without breaking compatibility
