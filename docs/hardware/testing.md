# Hardware Testing Guide

## Introduction

This guide provides comprehensive testing procedures to verify that your OpenRideDash hardware is functioning correctly. Proper testing ensures reliability, safety, and longevity of your system.

## Testing Philosophy

### Why Test?

- **Safety**: Prevent electrical hazards or malfunctions that could affect bike operation
- **Reliability**: Identify weak points before they fail in real-world use
- **Performance**: Ensure system meets design specifications
- **Quality**: Catch manufacturing defects early

### Testing Stages

1. **Component Testing**: Individual components before assembly
2. **Subsystem Testing**: After connecting subsystems (power, CAN, display)
3. **Integration Testing**: Complete system before potting
4. **Environmental Testing**: After enclosure assembly
5. **Final Validation**: Ready-for-use verification

## Test Equipment

### Essential Equipment

- **Multimeter**: Voltage, current, resistance measurements
- **Power Supply**: Variable voltage (0-80V) with current limiting
- **CAN Bus Analyzer**: CANable, PCAN-USB, or similar
- **Logic Analyzer**: For SPI, I²C signal analysis
- **Serial Terminal**: Computer with USB serial connection
- **Temperature Chamber**: Optional for thermal testing

### Optional Equipment

- **Oscilloscope**: Signal quality analysis
- **Thermal Camera**: Heat distribution visualization
- **Vibration Table**: Simulate handlebar mounting conditions
- **Water Spray Test**: IP rating verification
- **BLE Scanner**: nRF Connect or similar app

## Component Testing

### ESP32-C3 Testing

#### Basic Function Test

1. **Power Test**:
   - Connect 5V to VIN pin
   - Measure 3.3V output from regulator (should be 3.3V ±0.1V)
   - Check current draw (should be < 50mA idle)

2. **USB Test**:
   - Connect via USB
   - Verify serial communication at 115200 baud
   - Upload blink sketch to confirm programming works

3. **GPIO Test**:
   - Test each GPIO with simple input/output test
   - Verify pull-up/pull-down functionality
   - Check PWM capability on designated pins

#### Advanced Tests

1. **WiFi/BLE Test**:
   - Verify WiFi connection establishment
   - Test BLE advertising and connectivity
   - Measure RF performance (range, stability)

2. **CAN Controller Test**:
   - Test native CAN with loopback mode
   - Verify baud rate accuracy
   - Check error handling

### Buck Converter Testing

#### Static Tests

1. **Output Voltage**:
   - Apply 36V input (simulate typical battery voltage)
   - Measure output (should be 5.00V ±0.05V)
   - Adjust potentiometer if needed

2. **Efficiency**:
   - Apply 36V input, load with 500mA
   - Measure input current and output current
   - Calculate efficiency: η = (Vout × Iout) / (Vin × Iin)
   - Target: >85% efficiency

3. **Regulation**:
   - Sweep input voltage from 20V to 80V
   - Record output voltage at each step
   - Output should remain within 5.00V ±0.1V

#### Dynamic Tests

1. **Load Response**:
   - Apply step load (0mA → 500mA → 1000mA)
   - Measure output voltage transient
   - Should recover within 100ms with < 0.2V drop

2. **Overload Protection**:
   - Gradually increase load beyond rating
   - Verify current limiting or shutdown
   - Check for overheating

3. **Short Circuit Protection**:
   - Momentarily short output
   - Verify converter survives and recovers
   - Check for latch-up behavior

### CAN Transceiver Testing

#### Basic Tests

1. **Power Consumption**:
   - Measure quiescent current (should be < 10mA)
   - Measure transmit current (should be < 70mA)

2. **Signal Levels**:
   - Measure CAN_H and CAN_L voltage differential
   - Idle: 2.5V nominal
   - Dominant: CAN_H ~3.5V, CAN_L ~1.5V
   - Recessive: CAN_H ~2.5V, CAN_L ~2.5V

#### Communication Tests

1. **Loopback Test**:
   - Connect CAN_H to CAN_L via 120Ω resistor
   - Send message, verify reception
   - Test at multiple baud rates (125k, 250k, 500k, 1M)

2. **Error Handling**:
   - Inject errors (bit errors, format errors)
   - Verify error detection and reporting
   - Check recovery from bus-off state

### Display Testing

#### Visual Tests

1. **Pixel Test**:
   - Display test pattern (all pixels on)
   - Check for dead pixels or lines
   - Verify uniformity across screen

2. **Color Test** (if color display):
   - Display RGB test patterns
   - Verify color accuracy and saturation
   - Check for color bleed or cross-talk

3. **Refresh Test**:
   - Display moving pattern
   - Check for flicker or tearing
   - Verify smooth motion

#### Electrical Tests

1. **Power Consumption**:
   - Measure current with backlight off
   - Measure current with backlight at 50%
   - Measure current with backlight at 100%

2. **Signal Integrity**:
   - Analyze SPI signals with logic analyzer
   - Verify proper timing (clock frequency, data alignment)
   - Check for noise or signal degradation

### Keypad Testing

#### Mechanical Tests

1. **Button Operation**:
   - Press each button 100 times
   - Verify consistent operation
   - Check for sticking or failure

2. **Force Test**:
   - Measure actuation force (typical 2-5N)
   - Verify within comfortable range for riding

3. **Life Test**:
   - Automated pressing (1000 cycles)
   - Verify no degradation in performance

#### Electrical Tests

1. **Contact Resistance**:
   - Measure resistance when pressed (should be < 1Ω)
   - Measure resistance when released (should be > 1MΩ with pull-up)

2. **Debouncing**:
   - Analyze signal with oscilloscope
   - Verify bounce duration (< 10ms typical)
   - Check software debouncing effectiveness

## Subsystem Testing

### Power System Integration Test

#### Test Procedure

1. **Connect ESP32 to Buck Converter**
2. **Apply 36V input**
3. **Measure voltages at key points**:
   - Buck converter output (5V)
   - ESP32 VIN (5V)
   - ESP32 3.3V regulator output (3.3V)

4. **Load Test**:
   - Simulate full system load (connect display, CAN, etc.)
   - Measure total current draw
   - Verify voltage stability under load

5. **Transient Test**:
   - Rapidly switch load (simulate motor start)
   - Check for voltage dips or oscillations

#### Expected Results

- All voltages within specification
- Total current < 500mA typical, < 800mA maximum
- Voltage recovery within 50ms after load change
- No overheating of components

### CAN System Integration Test

#### Test Setup

1. **Connect ESP32 to CAN transceiver**
2. **Connect to CAN bus analyzer**
3. **Configure 500kbps baud rate**
4. **Add 120Ω termination if end-of-line**

#### Test Procedures

1. **Message Transmission**:
   - Send test messages from ESP32
   - Verify reception on analyzer
   - Check timing accuracy

2. **Message Reception**:
   - Send messages from analyzer
   - Verify ESP32 receives and processes
   - Check error counters

3. **Stress Test**:
   - High message rate (1000 messages/second)
   - Verify no message loss
   - Check for buffer overflows

4. **Error Injection**:
   - Inject CAN errors (bit errors, CRC errors)
   - Verify system handles gracefully
   - Check recovery time

#### Expected Results

- 100% message transmission/reception at normal rates
- < 1% loss at stress test rates
- Error counters increment appropriately
- System recovers from errors within 100ms

### Display System Integration Test

#### Test Setup

1. **Connect display to ESP32**
2. **Upload display test firmware**
3. **Connect logic analyzer to SPI lines**

#### Test Procedures

1. **Initialization Test**:
   - Verify display initializes correctly
   - Check initialization sequence timing
   - Verify no communication errors

2. **Drawing Test**:
   - Draw test patterns (lines, circles, text)
   - Verify accuracy and speed
   - Check for artifacts or corruption

3. **Update Rate Test**:
   - Measure frame update rate
   - Verify meets target (30Hz typical)
   - Check for consistent timing

4. **Power Management Test**:
   - Test backlight control (PWM)
   - Verify smooth brightness adjustment
   - Test display sleep/wake cycles

#### Expected Results

- Display initializes within 2 seconds
- Drawing operations complete within expected times
- Update rate ≥ 30Hz
- Backlight control smooth and responsive

### Keypad System Integration Test

#### Test Setup

1. **Connect keypad to ESP32**
2. **Upload keypad test firmware**
3. **Connect oscilloscope to GPIO lines**

#### Test Procedures

1. **Button Detection Test**:
   - Press each button individually
   - Verify correct GPIO detection
   - Check for cross-talk between buttons

2. **Multi-button Test**:
   - Press multiple buttons simultaneously
   - Verify all detected correctly
   - Check for matrix conflicts (if using matrix)

3. **Long-Press Test**:
   - Hold buttons for varying durations
   - Verify accurate timing detection
   - Check threshold consistency

4. **Debounce Test**:
   - Analyze button signals with oscilloscope
   - Verify bounce suppression
   - Measure effective debounce time

#### Expected Results

- All buttons detected correctly
- No false positives or ghost presses
- Long-press detection accurate (±50ms)
- Debouncing effective (no multiple triggers)

## Integration Testing

### Complete System Test

#### Test Setup

1. **Assemble all subsystems** (power, CAN, display, keypad)
2. **Connect to test bench**:
   - Variable power supply (36V-52V)
   - CAN bus simulator
   - Serial monitor
   - Load resistors (simulate sensors)

3. **Upload comprehensive test firmware**

#### Test Procedures

1. **Power-Up Sequence**:
   - Apply power
   - Monitor startup sequence
   - Verify all subsystems initialize correctly
   - Measure startup time (< 3 seconds target)

2. **Functional Test**:
   - Test CAN communication with simulator
   - Test display updates with live data
   - Test keypad interaction
   - Test BLE advertising (if enabled)

3. **Stress Test**:
   - High CAN message rate
   - Rapid display updates
   - Continuous button presses
   - Monitor for stability over 30 minutes

4. **Error Recovery Test**:
   - Simulate CAN bus errors
   - Simulate display communication errors
   - Simulate power interruptions
   - Verify graceful recovery

#### Expected Results

- System starts within 3 seconds
- All functions operational
- Stable under stress conditions
- Proper error handling and recovery
- No memory leaks or resource exhaustion

### Thermal Testing

#### Test Setup

1. **Place system in temperature controlled environment**
2. **Monitor temperatures with thermocouples**:
   - ESP32 die temperature
   - Buck converter temperature
   - Display temperature
   - Ambient temperature

3. **Apply typical load**

#### Test Procedures

1. **Room Temperature Test** (25°C):
   - Run for 1 hour
   - Record temperature rise
   - Verify within safe limits

2. **High Temperature Test** (50°C):
   - Gradually increase ambient temperature
   - Run for 1 hour at 50°C
   - Verify operation and temperature stability

3. **Low Temperature Test** (-10°C):
   - Gradually decrease ambient temperature
   - Run for 1 hour at -10°C
   - Verify operation and startup capability

4. **Thermal Cycling**:
   - Cycle between -10°C and 50°C
   - 10 cycles, 1 hour per extreme
   - Verify no degradation

#### Expected Results

- ESP32 temperature < 85°C at 50°C ambient
- Buck converter temperature < 100°C at full load
- Display operational at all temperatures
- No performance degradation after cycling

### Power Consumption Testing

#### Test Setup

1. **Connect precision current meter**
2. **Set up different operational modes**
3. **Measure current at each mode**

#### Test Procedures

1. **Sleep Mode**:
   - Enter deep sleep
   - Measure current (target < 100µA)

2. **Idle Mode**:
   - System running but no activity
   - Measure current (target < 150mA)

3. **Active Mode**:
   - Normal operation (CAN active, display updating)
   - Measure current (target < 250mA)

4. **Peak Mode**:
   - All subsystems active (BLE, display max brightness, CAN high rate)
   - Measure current (target < 500mA)

#### Expected Results

- Sleep current < 100µA
- Idle current < 150mA
- Active current < 250mA
- Peak current < 500mA

## Environmental Testing

### Water Resistance Testing

#### Test Setup

1. **Assemble enclosed system**
2. **Prepare water spray equipment**
3. **Connect to power and monitor operation**

#### Test Procedures

1. **Spray Test** (IP65 equivalent):
   - Spray water from all directions
   - 1 liter/minute for 5 minutes
   - Verify no water ingress
   - Continue operation during test

2. **Immersion Test** (optional):
   - Partial immersion (connectors submerged)
   - 30 minutes at 1 meter depth
   - Verify no water ingress
   - Dry thoroughly before further testing

3. **Condensation Test**:
   - Cycle between high and low temperature
   - Create condensation conditions
   - Verify no fogging or moisture inside

#### Expected Results

- No water ingress during spray test
- Operation continues during testing
- No condensation issues
- Connectors remain waterproof

### Vibration Testing

#### Test Setup

1. **Mount system on vibration table**
2. **Simulate handlebar mounting conditions**
3. **Monitor operation during vibration**

#### Test Procedures

1. **Sine Wave Vibration**:
   - 5-100Hz sweep at 0.5g amplitude
   - 30 minutes duration
   - Verify no loose connections or failures

2. **Random Vibration**:
   - Simulate road vibration profile
   - 1 hour duration
   - Verify operation stability

3. **Impact Test**:
   - Simulate drop from handlebar height (1 meter)
   - 3 drops onto padded surface
   - Verify no physical damage or functional failure

#### Expected Results

- No component detachment or loose connections
- Operation stable during vibration
- No physical damage from impacts
- Connectors remain secure

### UV Exposure Testing

#### Test Setup

1. **Place in UV exposure chamber**
2. **Simulate sunlight exposure**
3. **Monitor material degradation**

#### Test Procedures

1. **Accelerated UV Test**:
   - 100 hours UV exposure
   - Monitor enclosure color and integrity
   - Verify no cracking or deformation

2. **Thermal UV Combination**:
   - UV exposure with temperature cycling
   - Simulate real-world conditions
   - Verify material stability

#### Expected Results

- No significant color fading
- No cracking or deformation
- Materials remain structurally sound
- Display readability unaffected

## Final Validation

### Real-World Simulation

#### Test Setup

1. **Connect to e-bike or simulator**
2. **Simulate typical riding conditions**
3. **Monitor over extended period**

#### Test Procedures

1. **Day-long Test**:
   - 8 hours continuous operation
   - Simulate riding patterns (start/stop, varying load)
   - Monitor stability and performance

2. **Multi-day Test**:
   - 3 days intermittent operation
   - Simulate daily commuting pattern
   - Verify reliability over time

3. **Edge Case Testing**:
   - Low battery voltage (20V)
   - High battery voltage (80V)
   - Rapid temperature changes
   - High humidity conditions

#### Expected Results

- Stable 8+ hour operation
- No failures over multi-day test
- Handles edge cases gracefully
- Performance consistent throughout

### Certification Testing (Optional)

#### EMI/EMC Testing

1. **Radiated Emissions**: Verify compliance with limits
2. **Conducted Emissions**: Verify power line noise within limits
3. **Immunity**: Verify resistance to external interference

#### Safety Testing

1. **Electrical Safety**: Verify isolation, protection circuits
2. **Mechanical Safety**: Verify secure mounting, no sharp edges
3. **Thermal Safety**: Verify no overheating hazards

## Test Documentation

### Test Records

Keep detailed records of all tests:

1. **Test Configuration**: Hardware versions, firmware versions
2. **Test Conditions**: Temperature, voltage, load conditions
3. **Test Results**: Measurements, observations, issues
4. **Test Duration**: Start/end times, total duration
5. **Test Personnel**: Who performed the tests

### Test Reports

Generate test reports for:

1. **Component Testing**: Individual component verification
2. **Integration Testing**: System-level verification
3. **Environmental Testing**: Durability verification
4. **Final Validation**: Ready-for-use verification

## Troubleshooting Test Failures

### Common Test Failures

#### Power System Failures

- **Voltage regulation failure**: Check buck converter, adjust potentiometer
- **Overheating**: Improve heat sinking, reduce load
- **Current limiting**: Check for shorts, verify component ratings

#### Communication Failures

- **CAN communication failure**: Check termination, baud rate, wiring
- **Display communication failure**: Check SPI connections, verify timing
- **BLE communication failure**: Check antenna, verify RF environment

#### Mechanical Failures

- **Button failure**: Check wiring, verify mechanical operation
- **Mounting failure**: Improve mounting method, verify stability
- **Water ingress**: Improve sealing, verify connector waterproofing

### Failure Analysis Process

1. **Isolate Failure**: Determine which subsystem failed
2. **Root Cause Analysis**: Identify underlying cause
3. **Corrective Action**: Implement fix
4. **Retest**: Verify fix resolves issue
5. **Document**: Update test records with findings

## Test Automation

### Automated Test Scripts

Consider developing automated test scripts for:

1. **Power-up testing**: Automated voltage/current measurements
2. **Communication testing**: Automated CAN message testing
3. **Display testing**: Automated pattern generation and verification
4. **Keypad testing**: Automated button press simulation

### Continuous Testing

Implement continuous testing for:

1. **Regression testing**: After firmware changes
2. **Production testing**: For manufactured units
3. **Quality assurance**: Regular verification of standards

## Next Steps After Testing

Once testing is complete:

1. **Document successful tests** for future reference
2. **Address any issues** found during testing
3. **Proceed to firmware installation** if hardware passes
4. **Share test results** with community for feedback
5. **Consider certification** if required for your application

## Additional Resources

- **[Test Equipment Recommendations](https://github.com/open-ride-dash/hardware/blob/main/test-equipment.md)**
- **[Test Case Repository](https://github.com/open-ride-dash/firmware/tree/main/test)**
- **[Community Test Results](https://forum.open-ride-dash.org/c/testing)**
- **[Professional Testing Services](https://forum.open-ride-dash.org/c/testing-services)**

---

_Thorough testing is the key to a reliable OpenRideDash system. Invest time in proper testing to ensure your build will perform well in real-world conditions._
