# ORD Dash — BLE NUS Text API Protocol

Authoritative reference for the Nordic UART Service (NUS) command/reply protocol
used by the OpenRideDash (ORD) e-bike display.

---

## 1. Transport

### NUS Service

| Role | UUID | Property |
|------|------|----------|
| NUS Service | `6e400001-b5a3-f393-e0a9-e50e24dcca9e` | — |
| RX (phone → device) | `6e400002-b5a3-f393-e0a9-e50e24dcca9e` | **WRITE** (with response) |
| TX (device → phone) | `6e400003-b5a3-f393-e0a9-e50e24dcca9e` | **NOTIFY** only (no READ) |

### RX (phone → device)

The phone writes a UTF-8 command string (max 250 bytes) to the RX characteristic.
The device processes the command asynchronously and enqueues a reply on the TX
characteristic.

### TX (device → phone) — Fragmentation

Replies are sent as one or more BLE notifications. The first notification carries
a **2-byte big-endian total-length prefix**; subsequent notifications are raw
continuation data.

```
First notification:  [len_hi][len_lo][payload bytes 0..N-1]
Subsequent:          [payload bytes N..M-1]
                     ...
```

- `len_hi` = `(total >> 8) & 0xFF`
- `len_lo` = `total & 0xFF`
- `total` = total number of payload bytes (excluding the 2-byte prefix)
- Chunk size = `min(MTU, 247) - 3` (ATT overhead). At MTU=247, each notification
  carries 244 bytes of payload.
- 10 ms delay between multi-chunk frames.

**Phone reassembly rule:** Read the first 2 bytes as big-endian total length,
then accumulate notifications until that many bytes have been received. The first
notification includes the 2-byte prefix (so first payload = `first.length - 2`).

---

## 2. Command Format

Commands are UTF-8 strings sent to the NUS RX characteristic. The format is:

```
<command>[ <args>]
```

- `<command>` — a single word (max 32 chars), case-sensitive.
- `<args>` — optional, space-delimited tokens following the command name.
- Max total length: 250 bytes (hardware limit).
- Trailing `\r` / `\n` are stripped by the device.

### Argument structure

Commands follow a consistent pattern:

| Pattern | Example |
|---------|---------|
| `cmd` (bare) | `hostname` — get current value |
| `cmd <value>` | `hostname ord-c3` — set value |
| `cmd <subcommand>` | `ble on`, `ble status` — subcommand action |
| `cmd <subcommand> <value>` | `wifi ssid MyNetwork` — subcommand with value |

Boolean values use the strings `on` (true) and `off` (false), as returned by
`Util::boolToString()`.

---

## 3. Reply Format

Every command produces a reply with the following wire format:

```
API [<command>[ <args>]] (<code>) <data>
```

| Part | Description |
|------|-------------|
| `API [` | Literal prefix |
| `<command>` | The command name (e.g. `hostname`, `ble`) |
| ` ` | Space separator (present only if args follow) |
| `<args>` | The arguments as received by the handler |
| `] (` | Literal separator |
| `<code>` | Reply code string (see below) |
| `) ` | Literal separator |
| `<data>` | Reply payload (text, may be empty) |

### Reply codes

| Code | Enum value | Meaning |
|------|-----------|---------|
| `Success` | `Code::Success` | Command executed successfully |
| `Unknown Command` | `Code::UnknownCommand` | Command name not recognised |
| `Invalid Arguments` | `Code::InvalidArgs` | Arguments are malformed or out of range |
| `Execution Error` | `Code::ExecutionError` | Command failed at runtime |

### Examples

```
API [hostname] (Success) ord-c3
API [ble status] (Success) enabled: on, connected: true
API [wifi] (Success) sta: on, ap: on, ssid: myNetwork, password: myPassword
API [battery] (Success) 720
API [v] (Success) ord-v0.1.0-42-gabc1234
API [ble bogus] (Invalid Arguments) Usage: ble[ on|off|toggle|status]
API [nonexistent] (Unknown Command) 'nonexistent' is a mystery
```

---

## 4. Commands

### 4.1 `v` — Version

| | |
|---|---|
| **Get** | `v` |
| **Reply data** | Build info string (`whoami`) |
| **Example** | `API [v] (Success) ord-v0.1.0-42-gabc1234` |

### 4.2 `help` — Help

| | |
|---|---|
| **List** | `help` |
| **Detail** | `help <command>` |
| **Reply data (list)** | `Commands: v, help, restart, ...` |
| **Reply data (detail)** | `<command>: <helpText>` |
| **Example** | `API [help] (Success) Commands: v, help, restart, ...` |

### 4.3 `restart` — Restart

| | |
|---|---|
| **Execute** | `restart` |
| **Reply** | (never returns — device reboots immediately) |

### 4.4 `hostname` — Hostname

| | |
|---|---|
| **Get** | `hostname` |
| **Set** | `hostname <name>` |
| **Reply data (get)** | Current hostname string |
| **Reply data (set)** | New hostname string (may be trimmed) |
| **Example** | `API [hostname] (Success) ord-c3` |

### 4.5 `battery` — Battery capacity

| | |
|---|---|
| **Get** | `battery` |
| **Set** | `battery <capacity>` |
| **Args** | `<capacity>` — uint16, watt-hours |
| **Reply data (get)** | Current capacity in Wh |
| **Reply data (set)** | `Battery capacity set to <N> Wh` |
| **Example** | `API [battery] (Success) 720` |

### 4.6 `ble` — BLE radio

| | |
|---|---|
| **Status** | `ble` or `ble status` |
| **Enable** | `ble on` — **triggers reboot** |
| **Disable** | `ble off` — **triggers reboot** |
| **Toggle** | `ble toggle` — **triggers reboot** |
| **Reply data (status)** | `enabled: <on|off>, connected: <on|off>` |
| **Reply data (on/off/toggle)** | `<on|off>` — new state after applying change |
| **Example** | `API [ble] (Success) enabled: on, connected: true` |

### 4.7 `wifi` — WiFi

| | |
|---|---|
| **Summary** | `wifi` |
| **Enable STA** | `wifi on` — **triggers reboot** |
| **Disable STA** | `wifi off` — **triggers reboot** |
| **Toggle STA** | `wifi toggle` — **triggers reboot** |
| **Get SSID** | `wifi ssid` |
| **Set SSID** | `wifi ssid <name>` — restarts STA |
| **Get password** | `wifi password` |
| **Set password** | `wifi password <pw>` — restarts STA |
| **Get AP state** | `wifi ap` |
| **Enable AP** | `wifi ap on` — **triggers reboot** |
| **Disable AP** | `wifi ap off` — **triggers reboot** |
| **Toggle AP** | `wifi ap toggle` — **triggers reboot** |
| **Connection status** | `wifi status` |
| **Reply data (summary)** | `sta: <on|off>, ap: <on|off>, ssid: <name>, password: <pw>` |
| **Reply data (on/off/toggle)** | `<on|off>` — new state |
| **Reply data (ssid/password)** | Current value string |
| **Reply data (status)** | `sta: <connected|disconnected>, ap_clients: <N>` |
| **Example** | `API [wifi] (Success) sta: on, ap: on, ssid: myNetwork, password: myPassword` |

### 4.8 `sim` — Simulator (only if `FEATURE_SIM` is compiled)

| | |
|---|---|
| **Status** | `sim` |
| **Enable** | `sim on` |
| **Disable** | `sim off` |
| **Reply data** | `sim <on|off>` |
| **Example** | `API [sim] (Success) sim on` |

### 4.9 `state` — JSON telemetry snapshot

| | |
|---|---|
| **Get** | `state` |
| **Reply data** | JSON object with current telemetry values |
| **Example** | `API [state] (Success) {"speed":0.00,"pas":0,"torque":750,...}` |

### 4.10 `echo` — WifiSerial echo (only if WifiSerial is active)

| | |
|---|---|
| **Enable** | `echo on` |
| **Disable** | `echo off` |
| **Reply data** | `WifiSerial echo <on|off>` |

### 4.11 `key` — Simulate key press

| | |
|---|---|
| **Execute** | `key <up|down|upLong|downLong|power|menu>` |
| **Reply** | (triggers UI event, no meaningful data) |

### 4.12 `arm` / `disarm` — Motion alarm

| | |
|---|---|
| **Arm** | `arm` |
| **Disarm** | `disarm` |
| **Reply** | (arms/disarms MPU motion sensor) |

### 4.13 `nullpointer` — Crash test

| | |
|---|---|
| **Execute** | `nullpointer` |
| **Reply** | (crashes the device intentionally) |

---

## 5. Boolean representation

All boolean values use the strings `on` (true) and `off` (false), as defined by
`Util::boolToString()`:

```cpp
static const char* boolToString(bool value) {
    return value ? "on" : "off";
}
```

Parsing helpers:

```cpp
static bool isStrBoolOn(const char* str)  { return strcmp(str, "on") == 0; }
static bool isStrBoolOff(const char* str) { return strcmp(str, "off") == 0; }
static bool parseBoolValue(const char* args, bool* value);
```

---

## 6. CTS Telemetry Payload

The CTS custom service (`f6333d96-74c0-462d-b92d-5750a2283429`) sends a 14-byte
little-endian binary payload on the telemetry characteristic
(`5ee460d2-75a3-41ac-9034-2b2d435bb549`, NOTIFY + READ).

| Byte | Field | Type | Scale |
|------|-------|------|-------|
| 0 | Version/flags | `uint8` | `0x01` |
| 1–2 | Speed | `uint16` LE | km/h × 100 |
| 3–4 | Battery voltage | `uint16` LE | V × 100 |
| 5–6 | Battery current | `uint16` LE | A × 100 (unsigned — no regen) |
| 7 | State of charge | `uint8` | % |
| 8–9 | Range | `uint16` LE | km × 100 |
| 10 | PAS level | `int8` | −1 walk, 0 off, 1–5 |
| 11–12 | Human power | `uint16` LE | W × 10 |
| 13 | Cadence | `uint8` | RPM |

Forward-compat: ignore trailing bytes if `length > 14`; reject unknown version.

---

## 7. CTS HR Write Characteristic

The CTS service also exposes a write characteristic for receiving heart rate data
from the phone:

| UUID | Property | Payload |
|------|----------|---------|
| `a2c4f7b1-0e3d-4a8c-9b6e-1f2c3d4e5f60` | **WRITE_NR** | Single `uint8` BPM |

The phone writes the current heart rate (BPM) from a connected BLE HRM. The
device stores it in `state.heartRate()` and surfaces it on the display.

---

## 8. Consistency notes

- **Every command** follows the pattern `cmd[ subcmd][ value]`.
- **Every reply** uses the fixed wire format `API [<cmd> <args>] (<code>) <data>`.
- **Every boolean** in reply data uses `on`/`off` from `Util::boolToString()`.
- **Set commands** that change a value return the new value in the reply data.
- **Get commands** (bare command, no args) return the current value.
- **Reboot-triggering commands** (`ble on|off|toggle`, `wifi on|off|toggle`,
  `wifi ap on|off|toggle`) never return a reply — the device restarts immediately.
