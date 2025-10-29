/*
 * BL0940 Energy Meter IC - SPI Interface
 * Author: Christopher Mendez Martinez | @mcmchris
 * Modified by: Renatto Marchiori
 * Date: 2025
 * 
 * Calibration-free single-phase energy metering IC
 * Features: Current, Voltage, Power, Energy, Temperature measurement
 * Interface: SPI (MODE0, up to 900kHz)
 */

#include <Arduino.h>
#include "MCM_BL0940.h"

// SPI Commands
#define BL0940_READ       0x58
#define BL0940_WRITE      0xA8

// Registers
#define I_RMS             0x04
#define V_RMS             0x06
#define WATT              0x08
#define CF_CNT            0x0A
#define CORNER            0x0C
#define TPS1              0x0E
#define MODE              0x18
#define SOFT_RESET        0x19
#define USR_WRPROT        0x1A

// SPI Settings: MODE0 (CPOL=0, CPHA=0), 400kHz
SPISettings BL0940::spiSettings = SPISettings(400000, MSBFIRST, SPI_MODE0);

// ========== Constructor ==========

BL0940::BL0940(int8_t selPin, int8_t sckPin, int8_t misoPin, int8_t mosiPin)
{
  _selPin = selPin;
  _sckPin = sckPin;
  _misoPin = misoPin;
  _mosiPin = mosiPin;
}

BL0940::~BL0940()
{
  SPI.beginTransaction(spiSettings);
  for(int i = 0; i < 6; i++) {
    SPI.transfer(0xFF);
  }
  SPI.endTransaction();
}

// ========== Initialization ==========

bool BL0940::begin(uint32_t spiFrequency)
{
  pinMode(_selPin, OUTPUT);
  digitalWrite(_selPin, HIGH);  // HIGH = SPI mode
  pinMode(_misoPin, INPUT_PULLUP);
  
  delay(100);
  
  SPI.begin(_sckPin, _misoPin, _mosiPin, -1);
  spiSettings = SPISettings(400000, MSBFIRST, SPI_MODE0);
  
  delay(200);
  
  Reset();
  delay(500);
  
  uint32_t testData;
  return _readRegister(V_RMS, &testData);
}

// ========== Checksum ==========

uint8_t BL0940::_calcChecksum(uint8_t cmd, uint8_t addr, uint8_t data_h, uint8_t data_m, uint8_t data_l)
{
  return ~((cmd + addr + data_h + data_m + data_l) & 0xFF);
}

// ========== Write Register ==========

void BL0940::_writeRegister(uint8_t addr, uint32_t data)
{
  uint8_t data_h = (data >> 16) & 0xFF;
  uint8_t data_m = (data >> 8) & 0xFF;
  uint8_t data_l = data & 0xFF;
  uint8_t checksum = _calcChecksum(BL0940_WRITE, addr, data_h, data_m, data_l);
  
  SPI.beginTransaction(spiSettings);
  SPI.transfer(BL0940_WRITE);
  SPI.transfer(addr);
  SPI.transfer(data_h);
  SPI.transfer(data_m);
  SPI.transfer(data_l);
  SPI.transfer(checksum);
  SPI.endTransaction();
}

// ========== Read Register ==========

bool BL0940::_readRegister(uint8_t addr, uint32_t *data)
{
  const uint8_t MAX_RETRIES = 3;
  
  for(uint8_t retry = 0; retry < MAX_RETRIES; retry++)
  {
    SPI.beginTransaction(spiSettings);
    
    SPI.transfer(BL0940_READ);
    SPI.transfer(addr);
    delayMicroseconds(1200);
    
    uint8_t data_h = SPI.transfer(0xFF);
    uint8_t data_m = SPI.transfer(0xFF);
    uint8_t data_l = SPI.transfer(0xFF);
    uint8_t checksum = SPI.transfer(0xFF);
    
    SPI.endTransaction();
    
    uint8_t expected = _calcChecksum(BL0940_READ, addr, data_h, data_m, data_l);
    
    if (checksum == expected) {
      *data = ((uint32_t)data_h << 16) | ((uint32_t)data_m << 8) | (uint32_t)data_l;
      return true;
    }
    
    delayMicroseconds(500);
  }
  
  return false;
}

// ========== Voltage ==========

bool BL0940::getVoltage(float *voltage)
{
  uint32_t data;
  if (!_readRegister(V_RMS, &data)) {
    return false;
  }
  
  // R_total em ohms (convertido de kΩ), então dividimos por 1000
  double R_total = (double)(R2 + R9 + R10 + R19 + R20) / 1000.0;
  *voltage = (float)((double)data * Vref * R_total / (79931.0 * R30));
  
  return true;
}

// ========== Current ==========

bool BL0940::getCurrent(float *current)
{
  uint32_t data;
  if (!_readRegister(I_RMS, &data)) {
    return false;
  }
  
  double result = ((double)data * Vref * Rt) / (324004.0 * R29 * 1000.0);
  *current = (float)result * CURRENT_CAL;
  
  return true;
}

// ========== Active Power ==========

bool BL0940::getActivePower(float *activePower)
{
  uint32_t data;
  if (!_readRegister(WATT, &data)) {
    return false;
  }
  
  int32_t rawPower = (int32_t)(data << 8) >> 8;
  if (rawPower < 0) rawPower = -rawPower;
  
  // R_total em ohms (convertido de kΩ), então dividimos por 1000
  double R_total = (double)(R2 + R9 + R10 + R19 + R20) / 1000.0;
  double result = ((double)rawPower * Vref * Vref * R_total) / 
                  (4046.0 * (R29 * 1000.0 / Rt) * R30);
  
  *activePower = (float)result * POWER_CAL;
  
  return true;
}

// ========== Active Energy ==========

bool BL0940::getActiveEnergy(float *activeEnergy)
{
  uint32_t data;
  if (!_readRegister(CF_CNT, &data)) {
    return false;
  }
  
  int32_t rawCF = (int32_t)(data << 8) >> 8;
  if (rawCF < 0) rawCF = -rawCF;
  
  // R_total em ohms (convertido de kΩ), então dividimos por 1000
  double R_total = (double)(R2 + R9 + R10 + R19 + R20) / 1000.0;
  *activeEnergy = (float)rawCF * 1638.4 * 256.0 * Vref * Vref * R_total / 
                  (3600000.0 * 4046.0 * (R29 * 1000.0 / Rt) * R30);
  
  return true;
}

// ========== Power Factor ==========

bool BL0940::getPowerFactor(float *powerFactor)
{
  uint32_t data;
  if (!_readRegister(CORNER, &data)) {
    return false;
  }
  
  float rawPF = cos(2.0 * PI * (float)data * (float)Hz / 1000000.0);
  if (rawPF < 0) rawPF = -rawPF;
  
  *powerFactor = rawPF * 100.0;
  
  return true;
}

// ========== Temperature ==========

bool BL0940::getTemperature(float *temperature)
{
  uint32_t data;
  if (!_readRegister(TPS1, &data)) {
    return false;
  }
  
  int16_t rawTemp = (int16_t)(data << 6) / 64;
  *temperature = (170.0 / 448.0) * (rawTemp / 2.0 - 32.0) - 45;
  
  return true;
}

// ========== Configuration ==========

bool BL0940::setFrequency(uint32_t frequency)
{
  uint32_t data;
  if (!_readRegister(MODE, &data)) return false;
  
  if (frequency == 50) {
    data &= ~0b0000001000000000;
    Hz = 50.0;
  } else {
    data |= 0b0000001000000000;
    Hz = 60.0;
  }
  
  _writeRegister(MODE, data);
  return true;
}

bool BL0940::setUpdateRate(uint32_t rate)
{
  uint32_t data;
  if (!_readRegister(MODE, &data)) return false;
  
  if (rate == 400) {
    data &= ~0b0000000100000000;
    updateRate = 400;
  } else {
    data |= 0b0000000100000000;
    updateRate = 800;
  }
  
  _writeRegister(MODE, data);
  return true;
}

bool BL0940::Reset()
{
  _writeRegister(SOFT_RESET, 0x5A5A5A);
  delay(500);
  return true;
}

// ========== Calibration ==========

void BL0940::setCurrentCalibration(float factor)
{
  CURRENT_CAL = factor;
}

void BL0940::setPowerCalibration(float factor)
{
  POWER_CAL = factor;
}