/*
 * BL0940 Configuration Structure
 * Define hardware constants and calibration values
 */

#ifndef BL0940_CONFIG_H
#define BL0940_CONFIG_H

struct BL0940Config {
  // Hardware Constants
  float Vref;    // [V] Internal voltage reference
  float R29;     // [Ohm] Current sense resistor (shunt)
  float R30;     // [Ohm] Current sense divider
  float Rt;      // Current sense total resistance
  float R2;      // [Ohm] Voltage divider
  float R9;      // [Ohm] Voltage divider
  float R10;     // [Ohm] Voltage divider
  float R19;     // [Ohm] Voltage divider
  float R20;     // [Ohm] Voltage divider
};

#endif  