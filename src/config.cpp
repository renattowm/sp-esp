#include "MCM_BL0940.h"

// Sua configuração global aqui
BL0940Config myConfig = {
  .Vref = 1.218,    // [V]
  .R29 = 3.9,       // [Ohm]
  .R30 = 24.0,      // [Ohm]
  .Rt = 2000.0,     // For 100A/50mA range
  .R2 = 33000.0,    // [Ohm] (33k)
  .R9 = 33000.0,    // [Ohm] (33k)
  .R10 = 33000.0,   // [Ohm] (33k)
  .R19 = 33000.0,   // [Ohm] (33k)
  .R20 = 33000.0    // [Ohm] (33k)
};