/*
*	BL0940 Energy Meter IC - SPI Version
*	Author: Christopher Mendez | @mcmchris
*	Modified for SPI: 2025
*	Based on Breno Diniz Trevisan's implementation
* Renatto's Marchiori library adjustments
*/

#pragma once

#include <Arduino.h>
#include <SPI.h>

#ifndef MCM_BL0940_h
#define MCM_BL0940_h

class BL0940
{
  public:
    BL0940(int8_t selPin, int8_t sckPin, int8_t misoPin, int8_t mosiPin);
    ~BL0940();
    
    bool begin(uint32_t spiFrequency = 400000);
    
    bool getCurrent(float *current);
    bool getVoltage(float *voltage);
    bool getActivePower(float *activePower);
    bool getActiveEnergy(float *activeEnergy);
    bool getPowerFactor(float *powerFactor);
    bool getTemperature(float *temperature);
    
    bool setFrequency(uint32_t Hz = 60);
    bool setUpdateRate(uint32_t rate = 400);
    bool Reset();

  private:
    // Pinos
    int8_t _selPin;
    int8_t _sckPin;
    int8_t _misoPin;
    int8_t _mosiPin;
    
    // SPI
    static SPISettings spiSettings;
    
    // Valores dos resistores
    const float Vref = 1.218;      // [V]
    const float R29 = 3.9;         // [Ohm] shunt
    const float Rt = 2000.0;       // 2000 to 100A/50mA and 800 to the 20A/25mA
    const float R2 = 33.0;         // [kOhm]
    const float R9 = 33.0;         // [kOhm]
    const float R10 = 33.0;        // [kOhm]
    const float R19 = 33.0;        // [kOhm]
    const float R20 = 33.0;        // [kOhm]
    const float R30 = 24.0;        // [Ohm]
    uint16_t updateRate = 400;     // [ms]
    double Hz = 60.0;              // used to calculate power factor
    
    // Métodos privados
    void _writeRegister(uint8_t addr, uint32_t data);
    bool _readRegister(uint8_t addr, uint32_t *data);
    uint8_t _calcChecksum(uint8_t cmd, uint8_t addr, uint8_t data_h, uint8_t data_m, uint8_t data_l);
};

#endif