/*
 * BL0940 Energy Meter IC - SPI Interface
 * Author: Christopher Mendez Martinez | @mcmchris
 * Modified by: Renatto Marchiori
 * Date: 2025
 */

#ifndef MCM_BL0940_H
#define MCM_BL0940_H

#include <Arduino.h>
#include <SPI.h>

class BL0940
{
public:
  // Constructor
  BL0940(int8_t selPin, int8_t sckPin, int8_t misoPin, int8_t mosiPin);
  ~BL0940();
  
  // Initialization
  bool begin(uint32_t spiFrequency = 400000);
  bool Reset();
  
  // Measurements
  bool getVoltage(float *voltage);
  bool getCurrent(float *current);
  bool getActivePower(float *activePower);
  bool getActiveEnergy(float *activeEnergy);
  bool getPowerFactor(float *powerFactor);
  bool getTemperature(float *temperature);
  
  // Configuration
  bool setFrequency(uint32_t frequency);
  bool setUpdateRate(uint32_t rate);
  
  // Calibration (adjust these for your hardware)
  void setCurrentCalibration(float factor);
  void setPowerCalibration(float factor);
  
  // Hardware constants (modify for your circuit)
  const float Vref = 1.218;    // [V] Internal voltage reference
  const float R29 = 3.9;       // [Ohm] Current sense resistor divider (shunt side)
  const float R30 = 24.0;      // [Ohm] Current sense resistor divider (ADC side)
  const float Rt = 2000.0;     // Current sense total resistance (2000 for 100A/50mA range)
  const float R2 = 33000.0;    // [Ohm] Voltage divider (converted from 33.0 kOhm)
  const float R9 = 33000.0;    // [Ohm] Voltage divider (converted from 33.0 kOhm)
  const float R10 = 33000.0;   // [Ohm] Voltage divider (converted from 33.0 kOhm)
  const float R19 = 33000.0;   // [Ohm] Voltage divider (converted from 33.0 kOhm)
  const float R20 = 33000.0;   // [Ohm] Voltage divider (converted from 33.0 kOhm)
  
  // Calibration factors (adjust to match real measurements)
  // For low loads: use higher factor (e.g. 1.05)
  // For high loads: use lower factor (e.g. 0.95)
  float CURRENT_CAL = 1.00;    // Current calibration multiplier
  float POWER_CAL = 1.00;      // Power calibration multiplier
  
  float Hz = 60.0;             // [Hz] AC line frequency (Brazil = 60Hz)
  uint32_t updateRate = 400;    // [ms] RMS update rate
  
private:
  int8_t _selPin;
  int8_t _sckPin;
  int8_t _misoPin;
  int8_t _mosiPin;
  
  static SPISettings spiSettings;
  
  uint8_t _calcChecksum(uint8_t cmd, uint8_t addr, uint8_t h, uint8_t m, uint8_t l);
  void _writeRegister(uint8_t addr, uint32_t data);
  bool _readRegister(uint8_t addr, uint32_t *data);
};

#endif