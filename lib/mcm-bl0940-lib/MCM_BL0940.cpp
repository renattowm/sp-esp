/*
*	BL0940 Energy Meter IC - SPI Version FINAL
*	Author: Christopher Mendez | @mcmchris
*   Modificaitions by Renatto Marchiori
*	Modified for SPI: 2025
*/

#include <Arduino.h>
#include "MCM_BL0940.h"

#define BL0940_DEBUG 1

// Definições
#define BL0940_READ       0x58
#define BL0940_WRITE      0xA8

// Registradores
#define I_RMS             0x04
#define V_RMS             0x06
#define WATT              0x08
#define CF_CNT            0x0A
#define CORNER            0x0C
#define TPS1              0x0E
#define MODE              0x18
#define SOFT_RESET        0x19
#define USR_WRPROT        0x1A

// IMPORTANTE: BL0940 requer SPI_MODE0 (CPOL=0, CPHA=0)
// MODE1 causa checksum offset de -1 byte
SPISettings BL0940::spiSettings = SPISettings(400000, MSBFIRST, SPI_MODE0);

// ========== CONSTRUTOR ==========

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

// ========== BEGIN ==========

bool BL0940::begin(uint32_t spiFrequency)
{
  Serial.println("\n╔════════════════════════════════╗");
  Serial.println("║   BL0940 SPI - MODE0          ║");
  Serial.println("╚════════════════════════════════╝");
  
  // CS/SEL HIGH = modo SPI
  pinMode(_selPin, OUTPUT);
  digitalWrite(_selPin, HIGH);
  pinMode(_misoPin, INPUT_PULLUP);
  
  Serial.printf("SEL:  GPIO %d → HIGH (SPI mode)\n", _selPin);
  Serial.printf("MISO: GPIO %d → Pull-up\n", _misoPin);
  Serial.printf("SCK:  GPIO %d\n", _sckPin);
  Serial.printf("MOSI: GPIO %d\n", _mosiPin);
  
  delay(100); // Aguardar chip entrar em modo SPI
  
  SPI.begin(_sckPin, _misoPin, _mosiPin, -1);
  
  uint32_t safeSpiFreq = 400000;
  spiSettings = SPISettings(safeSpiFreq, MSBFIRST, SPI_MODE0);
  
  Serial.printf("SPI: MODE0, %d Hz\n", safeSpiFreq);
  
  delay(200);
  
  Serial.println("\n→ Reset...");
  Reset();
  delay(500);
  
  Serial.println("→ Teste...");
  uint32_t testData;
  
  if(_readRegister(V_RMS, &testData)) {
    Serial.printf("V_RMS = 0x%06X\n", testData);
    Serial.println("════════════════════════════════\n");
    return true;
  }
  
  Serial.println("FALHA\n");
  return false;
}

// ========== CHECKSUM ==========

uint8_t BL0940::_calcChecksum(uint8_t cmd, uint8_t addr, uint8_t data_h, uint8_t data_m, uint8_t data_l)
{
  // Checksum padrão BL0940 SPI (INCLUI o comando 0x58 ou 0xA8)
  // Checksum = ~(cmd + addr + data_h + data_m + data_l)
  return ~((cmd + addr + data_h + data_m + data_l) & 0xFF);
}

// ========== WRITE ==========

void BL0940::_writeRegister(uint8_t addr, uint32_t data)
{
  uint8_t data_h = (data & 0x00FF0000) >> 16;
  uint8_t data_m = (data & 0x0000FF00) >> 8;
  uint8_t data_l = (data & 0x000000FF);
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

// ========== READ (EXATAMENTE COMO O BRUNO) ==========

bool BL0940::_readRegister(uint8_t addr, uint32_t *data)
{
  const uint8_t MAX_RETRIES = 3;
  
  for(uint8_t retry = 0; retry < MAX_RETRIES; retry++)
  {
    SPI.beginTransaction(spiSettings);
    
    // Enviar comando e endereço
    SPI.transfer(BL0940_READ);
    SPI.transfer(addr);
    
    // Half-duplex: aguardar chip processar (800µs otimizado)
    delayMicroseconds(800);
    
    // Ler resposta
    uint8_t data_h = SPI.transfer(0xFF);
    uint8_t data_m = SPI.transfer(0xFF);
    uint8_t data_l = SPI.transfer(0xFF);
    uint8_t checksum = SPI.transfer(0xFF);
    
    SPI.endTransaction();
    
    // Validar checksum
    uint8_t expected_checksum = _calcChecksum(BL0940_READ, addr, data_h, data_m, data_l);
    
    if (checksum != expected_checksum)
    {
      #if BL0940_DEBUG
      if(retry > 0) Serial.printf("[Retry %d] ", retry);
      Serial.printf("[0x%02X] ✗ CHK (exp:%02X recv:%02X)\n", addr, expected_checksum, checksum);
      #endif
      delayMicroseconds(500);
      continue;
    }
    
    // Dados válidos
    *data = ((uint32_t)data_h << 16) | ((uint32_t)data_m << 8) | (uint32_t)data_l;
    
    #if BL0940_DEBUG
    if(retry > 0) Serial.printf("[Retry %d] ", retry);
    Serial.printf("[0x%02X] → 0x%06X ✓\n", addr, *data);
    #endif
    
    return true;
  }
  
  // Falhou após retries
  #if BL0940_DEBUG
  Serial.printf("[0x%02X] ✗ FALHA após %d tentativas\n", addr, MAX_RETRIES);
  #endif
  
  return false;
}

// ========== LEITURAS ==========

bool BL0940::getVoltage(float *voltage)
{
  uint32_t data;
  if (!_readRegister(V_RMS, &data)) {
    return false;
  }
  
  *voltage = (float)data * Vref * (R2 + R9 + R10 + R19 + R20) / (79931.0 * R30);
  return true;
}

bool BL0940::getCurrent(float *current)
{
  uint32_t data;
  if (!_readRegister(I_RMS, &data)) {
    return false;
  }
  
  *current = (float)data * Vref / ((324004.0 * R29 * 1000.0) / Rt);
  return true;
}

bool BL0940::getActivePower(float *activePower)
{
  uint32_t data;
  if (!_readRegister(WATT, &data)) {
    return false;
  }
  
  int32_t rowActivePower = (int32_t)(data << 8) / 256;
  if (rowActivePower < 0)
    rowActivePower = -rowActivePower;
  
  *activePower = (float)rowActivePower * Vref * Vref * (R2 + R9 + R10 + R19 + R20) / 
                 (4046.0 * (R29 * 1000.0 / Rt) * R30);
  return true;
}

bool BL0940::getActiveEnergy(float *activeEnergy)
{
  uint32_t data;
  if (!_readRegister(CF_CNT, &data)) {
    return false;
  }
  
  int32_t rowCF_CNT = (int32_t)(data << 8) / 256;
  if (rowCF_CNT < 0)
    rowCF_CNT = -rowCF_CNT;
  
  *activeEnergy = (float)rowCF_CNT * 1638.4 * 256.0 * Vref * Vref * 
                  (R2 + R9 + R10 + R19 + R20) / 
                  (3600000.0 * 4046.0 * (R29 * 1000.0 / Rt) * R30);
  return true;
}

bool BL0940::getPowerFactor(float *powerFactor)
{
  uint32_t data;
  if (!_readRegister(CORNER, &data)) {
    return false;
  }
  
  float rowPowerFactor = cos(2.0 * PI * (float)data * (float)Hz / 1000000.0);
  if (rowPowerFactor < 0)
    rowPowerFactor = -rowPowerFactor;
  *powerFactor = rowPowerFactor * 100.0;
  return true;
}

bool BL0940::getTemperature(float *temperature)
{
  uint32_t data;
  if (!_readRegister(TPS1, &data)) {
    return false;
  }
  
  int16_t rowTemperature = (int16_t)(data << 6) / 64;
  *temperature = (170.0 / 448.0) * (rowTemperature / 2.0 - 32.0) - 45;
  return true;
}

// ========== CONFIG ==========

bool BL0940::setFrequency(uint32_t frequency)
{
  uint32_t data;
  if (!_readRegister(MODE, &data))
    return false;
  
  uint16_t mask = 0b0000001000000000;
  
  if (frequency == 50) {
    data &= ~mask;
    Hz = 50.0;
  } else {
    data |= mask;
    Hz = 60.0;
  }
  
  _writeRegister(MODE, data);
  Serial.printf("Freq: %.0f Hz\n", Hz);
  return true;
}

bool BL0940::setUpdateRate(uint32_t rate)
{
  uint32_t data;
  if (!_readRegister(MODE, &data))
    return false;
  
  uint16_t mask = 0b0000000100000000;
  
  if (rate == 400) {
    data &= ~mask;
    updateRate = 400;
  } else {
    data |= mask;
    updateRate = 800;
  }
  
  _writeRegister(MODE, data);
  Serial.printf("Rate: %d ms\n", updateRate);
  return true;
}

bool BL0940::Reset()
{
  _writeRegister(SOFT_RESET, 0x5A5A5A);
  delay(500);
  return true;
}