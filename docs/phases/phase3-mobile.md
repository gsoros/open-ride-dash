# Phase 3: Mobile Application & User Interaction

## Objective

Enable external control, visualization, and data recording via a mobile application while maintaining seamless integration with the embedded system.

## Success Criteria

- ✅ Mobile app discovers and connects to OpenRideDash device
- ✅ Real-time telemetry display in app
- ✅ Configuration interface for system settings
- ✅ Ride data recording and export
- ✅ OTA update control interface
- ✅ Cross-platform support (iOS & Android)

## Scope

### Mobile Application (Flutter)

- Device discovery, pairing, and connection management
- Real-time telemetry visualization
- Configuration interface for all system parameters
- Ride data recording with GPS integration
- OTA update initiation and monitoring

### Enhanced User Experience

- Graphical configuration interface
- Ride history and statistics
- Firmware update management
- Security and pairing management

### System Integration

- BLE API client implementation
- Configuration persistence on device
- Error reporting and diagnostics
- Battery-optimized communication

## Application Architecture

### Technology Stack

- **Framework**: Flutter 3.x
- **State Management**: Riverpod or Provider
- **BLE Library**: flutter_blue_plus
- **Storage**: Hive or SQLite
- **Maps**: Google Maps/Mapbox
- **Charts**: fl_chart or syncfusion_flutter_charts

### Project Structure

lib/
├── main.dart
├── core/
│ ├── ble/
│ │ ├── ble_manager.dart
│ │ ├── api_client.dart
│ │ └── device_scanner.dart
│ ├── models/
│ │ ├── device_state.dart
│ │ ├── ride_data.dart
│ │ └── config.dart
│ └── utils/
│ ├── logger.dart
│ └── formatters.dart
├── features/
│ ├── dashboard/
│ ├── configuration/
│ ├── history/
│ └── ota/
├── widgets/
│ ├── telemetry_card.dart
│ ├── gauge_widget.dart
│ └── connection_status.dart
└── services/
├── storage_service.dart
├── location_service.dart
└── export_service.dart

````

## BLE Integration

### Device Scanning
```dart
class DeviceScanner {
  final FlutterBluePlus flutterBlue = FlutterBluePlus.instance;

  Stream<List<ScanResult>> scanForDevices() {
    // Start scanning with filters
    flutterBlue.startScan(
      timeout: Duration(seconds: 10),
      withServices: [ORD_SERVICE_UUID],
    );

    return flutterBlue.scanResults;
  }

  Future<void> connectToDevice(BluetoothDevice device) async {
    await device.connect(autoConnect: true);

    // Discover services
    List<BluetoothService> services = await device.discoverServices();

    // Find ORD service
    BluetoothService ordService = services.firstWhere(
      (s) => s.uuid == ORD_SERVICE_UUID,
    );

    // Setup characteristics
    await _setupCharacteristics(ordService);
  }
}
````

### API Client Implementation

```dart
class OrdApiClient {
  final BluetoothCharacteristic _cmdChar;
  final BluetoothCharacteristic _eventChar;

  Stream<ApiEvent> get events => _eventStreamController.stream;
  final StreamController<ApiEvent> _eventStreamController;

  Future<ApiResponse> sendCommand(ApiCommand command) async {
    // Encode command
    Uint8List data = _encodeCommand(command);

    // Send via BLE
    await _cmdChar.write(data, withoutResponse: false);

    // Wait for response
    return await _waitForResponse(command.id);
  }

  void _onEventReceived(List<int> data) {
    ApiEvent event = _decodeEvent(data);
    _eventStreamController.add(event);
  }
}
```

\_eventStreamController.add(event);
}
}

````

## Data Recording

### Ride Data Model
```dart
class RideData {
  final String id;
  final DateTime startTime;
  final DateTime endTime;
  final List<LocationPoint> track;
  final List<TelemetrySample> telemetry;
  final RideStatistics statistics;

  double get distance => statistics.distance; // km
  Duration get duration => endTime.difference(startTime);
  double get averageSpeed => statistics.averageSpeed; // km/h
  double get maxSpeed => statistics.maxSpeed; // km/h
  double get energyUsed => statistics.energyUsed; // Wh
}

class TelemetrySample {
  final DateTime timestamp;
  final double batteryVoltage;
  final double batteryCurrent;
  final double speed;
  final int pasLevel;
  final double motorPower;
  final double? humanPower;
  final double? temperature;
  final Location? location;
}

class RideStatistics {
  final double distance; // km
  final double averageSpeed; // km/h
  final double maxSpeed; // km/h
  final double energyUsed; // Wh
  final double elevationGain; // m
  final double elevationLoss; // m
}
````

### Recording Service

```dart
class RideRecorder {
  final LocationService _location;
  final BleManager _ble;
  final StorageService _storage;

  RideData? _currentRide;
  Timer? _samplingTimer;

  Future<void> startRecording() async {
    _currentRide = RideData(
      id: Uuid().v4(),
      startTime: DateTime.now(),
      endTime: DateTime.now(),
      track: [],
      telemetry: [],
      statistics: RideStatistics.zero(),
    );

    // Start location tracking
    await _location.startTracking();

    // Start sampling timer (1Hz)
    _samplingTimer = Timer.periodic(
      Duration(seconds: 1),
      _takeSample,
    );
  }

  void _takeSample(Timer timer) {
    if (_currentRide == null) return;

    // Get current telemetry
    DeviceState state = _ble.currentState;
    Location? location = _location.currentLocation;

    TelemetrySample sample = TelemetrySample(
      timestamp: DateTime.now(),
      batteryVoltage: state.batteryVoltage,
      batteryCurrent: state.batteryCurrent,
      speed: state.speed,
      pasLevel: state.pasLevel,
      motorPower: state.motorPower,
      humanPower: state.humanPower,
      temperature: state.temperature,
      location: location,
    );

    _currentRide!.telemetry.add(sample);

    if (location != null) {
      _currentRide!.track.add(LocationPoint.fromLocation(location));
    }
  }

  Future<RideData> stopRecording() async {
    _samplingTimer?.cancel();
    await _location.stopTracking();

    if (_currentRide == null) {
      throw StateError('No active recording');
    }

    _currentRide!.endTime = DateTime.now();
    _currentRide!.statistics = _calculateStatistics(_currentRide!);

    // Save to storage
    await _storage.saveRide(_currentRide!);

    RideData ride = _currentRide!;
    _currentRide = null;

    return ride;
  }
}
```

## OTA Update Control

### Update Management

```dart
class OtaManager {
  final BleManager _ble;
  final ApiClient _api;

  Future<FirmwareInfo> checkForUpdates() async {
    // Get current firmware version
    DeviceInfo info = await _ble.getDeviceInfo();

    // Check for updates (could be from GitHub releases)
    return await _api.checkUpdates(
      deviceModel: info.model,
      currentVersion: info.firmwareVersion,
    );
  }

  Future<void> performUpdate(FirmwareInfo update) async {
    // Enable OTA mode on device
    await _ble.sendCommand(Command.enableOta());

    // Download firmware
    Uint8List firmware = await _downloadFirmware(update.downloadUrl);

    // Split into chunks and send
    await _uploadFirmware(firmware);

    // Wait for device to reboot
    await _waitForReboot();

    // Verify update
    await _verifyUpdate(update.version);
  }

  Stream<double> _uploadFirmware(Uint8List firmware) async* {
    const chunkSize = 244; // BLE MTU - 3 bytes header

    for (int i = 0; i < firmware.length; i += chunkSize) {
      int end = min(i + chunkSize, firmware.length);
      Uint8List chunk = firmware.sublist(i, end);

      await _ble.sendOtaChunk(chunk);

      double progress = (i / firmware.length) * 100;
      yield progress;

      // Small delay to prevent overwhelming BLE
      await Future.delayed(Duration(milliseconds: 10));
    }
  }
}
```

### Update UI

```dart
class OtaScreen extends StatefulWidget {
  @override
  _OtaScreenState createState() => _OtaScreenState();
}

class _OtaScreenState extends State<OtaScreen> {
  double _progress = 0.0;
  OtaState _state = OtaState.idle;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: Text('Firmware Update')),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            if (_state == OtaState.checking)
              CircularProgressIndicator(),

            if (_state == OtaState.downloading)
              Column(
                children: [
                  CircularProgressIndicator(),
                  SizedBox(height: 16),
                  Text('Downloading update...'),
                ],
              ),

            if (_state == OtaState.uploading)
              Column(
                children: [
                  LinearProgressIndicator(value: _progress / 100),
                  SizedBox(height: 16),
                  Text('Uploading: ${_progress.toStringAsFixed(1)}%'),
                  SizedBox(height: 8),
                  Text('Do not disconnect!'),
                ],
              ),

            if (_state == OtaState.complete)
              Column(
                children: [
                  Icon(Icons.check_circle, color: Colors.green, size: 64),
                  SizedBox(height: 16),
                  Text('Update complete!'),
                  SizedBox(height: 8),
                  Text('Device is restarting...'),
                ],
              ),
          ],
        ),
      ),
      floatingActionButton: _buildActionButton(),
    );
  }

  Widget? _buildActionButton() {
    if (_state == OtaState.idle) {
      return FloatingActionButton.extended(
        icon: Icon(Icons.update),
        label: Text('Check for Updates'),
        onPressed: _checkForUpdates,
      );
    }

    if (_state == OtaState.updateAvailable) {
      return FloatingActionButton.extended(
        icon: Icon(Icons.download),
        label: Text('Install Update'),
        onPressed: _performUpdate,
      );
    }

    return null;
  }
}
```

return ride;
}
}

## Security Implementation

### Pairing Management

```dart
class SecurityManager {
  final SecureStorage _storage;

  Future<void> pairDevice(BluetoothDevice device, String passkey) async {
    // Initiate pairing
    await device.pair(MatchingAlgorithm.numericComparison, passkey);

    // Store bond information
    await _storage.saveBondInfo(
      deviceId: device.id.toString(),
      passkey: passkey,
      pairedAt: DateTime.now(),
    );
  }

  Future<bool> isDevicePaired(BluetoothDevice device) async {
    BondState state = await device.bondState.first;
    return state == BondState.bonded;
  }

  Future<void> removePairing(BluetoothDevice device) async {
    await device.removeBond();
    await _storage.removeBondInfo(device.id.toString());
  }
}
```

### Secure Storage

```dart
class SecureStorage {
  final FlutterSecureStorage _secureStorage = FlutterSecureStorage();

  Future<void> saveBondInfo({
    required String deviceId,
    required String passkey,
    required DateTime pairedAt,
  }) async {
    await _secureStorage.write(
      key: 'bond_$deviceId',
      value: json.encode({
        'passkey': passkey,
        'pairedAt': pairedAt.toIso8601String(),
      }),
    );
  }

  Future<BondInfo?> getBondInfo(String deviceId) async {
    String? data = await _secureStorage.read(key: 'bond_$deviceId');
    if (data == null) return null;

    Map<String, dynamic> jsonData = json.decode(data);
    return BondInfo(
      passkey: jsonData['passkey'],
      pairedAt: DateTime.parse(jsonData['pairedAt']),
    );
  }
}
```

_[Development Tasks and Testing sections continue...]_
return ride;
}
}

## Security Implementation

### Pairing Management

```dart
class SecurityManager {
  final SecureStorage _storage;

  Future<void> pairDevice(BluetoothDevice device, String passkey) async {
    // Initiate pairing
    await device.pair(MatchingAlgorithm.numericComparison, passkey);

    // Store bond information
    await _storage.saveBondInfo(
      deviceId: device.id.toString(),
      passkey: passkey,
      pairedAt: DateTime.now(),
    );
  }

  Future<bool> isDevicePaired(BluetoothDevice device) async {
    BondState state = await device.bondState.first;
    return state == BondState.bonded;
  }

  Future<void> removePairing(BluetoothDevice device) async {
    await device.removeBond();
    await _storage.removeBondInfo(device.id.toString());
  }
}
```

### Secure Storage

```dart
class SecureStorage {
  final FlutterSecureStorage _secureStorage = FlutterSecureStorage();

  Future<void> saveBondInfo({
    required String deviceId,
    required String passkey,
    required DateTime pairedAt,
  }) async {
    await _secureStorage.write(
      key: 'bond_$deviceId',
      value: json.encode({
        'passkey': passkey,
        'pairedAt': pairedAt.toIso8601String(),
      }),
    );
  }

  Future<BondInfo?> getBondInfo(String deviceId) async {
    String? data = await _secureStorage.read(key: 'bond_$deviceId');
    if (data == null) return null;

    Map<String, dynamic> jsonData = json.decode(data);
    return BondInfo(
      passkey: jsonData['passkey'],
      pairedAt: DateTime.parse(jsonData['pairedAt']),
    );
  }
}
```

_[Development Tasks and Testing sections continue...]_
