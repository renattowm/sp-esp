# BL0940 Energy Meter - ESP32 SPI Library

A high-performance Arduino library for the **BL0940** energy meter IC using **SPI interface** on ESP32.

## Features

- Full SPI implementation (half-duplex communication)
- Automatic checksum validation with retry mechanism
- High accuracy measurements: voltage, current, power, energy, power factor, temperature
- Configurable calibration factors for current and power
- Clean API with error handling
- Optimized timing (800µs half-duplex delay, <40ms total read cycle)
- Debug logging with detailed SPI transaction info

## Measurements

| Parameter | Range | Accuracy |
|-----------|-------|----------|
| Voltage | 0-300V AC | ±0.5% |
| Current | 0-20A AC | ±1% |
| Power | 0-6000W | ±1% |
| Power Factor | 0-100% | ±2% |
| Energy | Accumulative | ±1% |
| Temperature | -40°C to 125°C | ±3°C |

## Hardware Setup

### Pin Configuration

```cpp
#define SEL_PIN  5    // CS/SEL (must be HIGH for SPI mode)
#define SCK_PIN  18   // SPI Clock
#define MISO_PIN 19   // Master In Slave Out (with pull-up)
#define MOSI_PIN 23   // Master Out Slave In
```

### Critical Requirements

**Important**: The BL0940 requires:

- **SPI_MODE0** (CPOL=0, CPHA=0) - MODE1 causes -1 byte checksum offset
- **SEL pin HIGH** throughout operation (enables SPI mode)
- **MISO pull-up** resistor (~10kΩ recommended)
- **Half-duplex delay** (min 800µs between command and response)

### Example Circuit

```text
ESP32                BL0940
──────               ──────
GPIO 5  ──────────── SEL (keep HIGH)
GPIO 18 ──────────── SCK
GPIO 19 ──────┬───── MISO
             │
            10kΩ
             │
            VCC

GPIO 23 ──────────── MOSI
GND     ──────────── GND
3.3V    ──────────── VCC
```

## Quick Start

### Installation

1. Clone or download this repository
2. Copy `lib/mcm-bl0940-lib/` to your PlatformIO project's `lib/` folder
3. Include the library in your code

### Basic Example

```cpp
#include <Arduino.h>
#include "MCM_BL0940.h"

// Pin definitions
#define SEL_PIN  5
#define SCK_PIN  18
#define MISO_PIN 19
#define MOSI_PIN 23

// Create meter instance
BL0940 meter(SEL_PIN, SCK_PIN, MISO_PIN, MOSI_PIN);

void setup() {
  Serial.begin(115200);
  
  // Initialize meter
  if(!meter.begin(400000)) {
    Serial.println("ERROR: BL0940 initialization failed!");
    while(1) delay(1000);
  }
  
  // Configure for 60Hz AC, 400ms update rate
  meter.setFrequency(60);
  meter.setUpdateRate(400);
  
  Serial.println("BL0940 ready!");
}

void loop() {
  float voltage, current, power, pf;
  
  // Read measurements
  if(meter.getVoltage(&voltage)) {
    Serial.printf("Voltage: %.2f V\n", voltage);
  }
  
  if(meter.getCurrent(&current)) {
    Serial.printf("Current: %.3f A\n", current);
  }
  
  if(meter.getActivePower(&power)) {
    Serial.printf("Power: %.2f W\n", power);
  }
  
  if(meter.getPowerFactor(&pf)) {
    Serial.printf("Power Factor: %.2f %%\n", pf);
  }
  
  delay(2000);
}
```

## Calibration

The library includes calibration factors to match your hardware:

### Hardware Resistors (modify in `MCM_BL0940.h`)

```cpp
float Vref = 1.218;    // Internal voltage reference
float R2 = 5.0;        // Voltage divider resistors
float R9 = 5.0;
float R10 = 5.0;
float R19 = 100.0;
float R20 = 50.0;
float R30 = 24.0;
float R29 = 3.9;       // Current sense resistor
float Rt = 2000.0;     // Current sense total resistance
```

### Software Calibration

Compare with a reference wattmeter and adjust:

```cpp
// If your readings are 1% low, use:
meter.setCurrentCalibration(1.01);
meter.setPowerCalibration(1.01);

// If your readings are 2% high, use:
meter.setCurrentCalibration(0.98);
meter.setPowerCalibration(0.98);
```

**Calibration procedure:**

1. Measure with reference wattmeter: `P_ref = 1840 W`
2. Read from BL0940: `P_measured = 1788 W`
3. Calculate factor: `factor = 1840 / 1788 = 1.029`
4. Apply: `meter.setPowerCalibration(1.029);`

## API Reference

### Initialization

```cpp
bool begin(uint32_t spiFrequency = 400000);
bool Reset();
```

### Measurements

```cpp
bool getVoltage(float *voltage);           // V
bool getCurrent(float *current);           // A
bool getActivePower(float *activePower);   // W
bool getActiveEnergy(float *activeEnergy); // kWh
bool getPowerFactor(float *powerFactor);   // % (0-100)
bool getTemperature(float *temperature);   // °C
```

All measurement functions return `true` on success, `false` on error.

### Configuration

```cpp
bool setFrequency(uint32_t frequency);     // 50 or 60 Hz
bool setUpdateRate(uint32_t rate);         // 400 or 800 ms
void setCurrentCalibration(float factor);  // 0.5 - 2.0
void setPowerCalibration(float factor);    // 0.5 - 2.0
```

## Debugging

Enable debug output by setting in `MCM_BL0940.cpp`:

```cpp
#define BL0940_DEBUG 1
```

Debug output shows:

- SPI transactions: `[0x06] → 0x20452C ✓`
- Checksum errors: `[0x08] ✗ CHK (exp:FE recv:FF)`
- Retry attempts: `[Retry 1] [0x04] → 0x3FE773 ✓`

## Performance

- **Single read**: ~1ms (with 800µs half-duplex delay)
- **Full cycle** (V+I+P+PF): ~35ms with debug, ~5ms without debug
- **Checksum validation**: 100% reliable with SPI_MODE0
- **Retry mechanism**: 3 attempts with 500µs delay

## Troubleshooting

### Checksum errors with -1 offset

**Cause**: Using SPI_MODE1 instead of MODE0  
**Solution**: Library uses MODE0 by default - do not change!

### All reads return 0xFFFFFF

**Cause**: SEL pin LOW or floating  
**Solution**: Ensure SEL pin is set HIGH and stays HIGH

### Intermittent failures

**Cause**: MISO pull-up missing, cable too long, EMI  
**Solution**: Add 10kΩ pull-up to MISO, use short wires (<15cm), add 0.1µF caps

### First read after begin() takes 11ms

**Cause**: Debug logging overhead on first transaction  
**Solution**: Normal behavior, subsequent reads are ~1ms

## License

Based on original work by Christopher Mendez Martinez (@mcmchris)  
Modified for SPI by Renatto Marchiori (2025)

## Contributing

Issues and pull requests welcome!

## References

- [BL0940 Datasheet](https://www.belling.com.cn/en/product/BL0940.html)
- [Original UART Library](https://github.com/mcmchris/BL0940-library)
- ESP32 SPI Documentation
