#include <Arduino.h>
#include "MCM_BL0940.h"

// Pinagem SPI do ESP32
#define BL0940_SEL_PIN  5   // Chip Select
#define BL0940_SCK_PIN  18  // Clock
#define BL0940_MISO_PIN 19  // Data In 
#define BL0940_MOSI_PIN 23  // Data Out

// Criar objeto BL0940
BL0940 meter(BL0940_SEL_PIN, BL0940_SCK_PIN, BL0940_MISO_PIN, BL0940_MOSI_PIN);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== BL0940 Test with Custom Config ===\n");
  
  // Criar configuração personalizada
  BL0940Config config;
  
  // Modificar valores padrão se necessário
  config.Vref = 1.218;    // [V] Reference voltage
  config.R29 = 3.9;       // [Ohm] Shunt resistor
  config.R30 = 24.0;      // [Ohm] Current sense divider
  config.Rt = 2000.0;     // Total resistance for 100A range
  
  // Divisor de tensão (todos em Ohm)
  config.R2 = 33000.0;    // 33k
  config.R9 = 33000.0;    // 33k
  config.R10 = 33000.0;   // 33k
  config.R19 = 33000.0;   // 33k
  config.R20 = 33000.0;   // 33k
  
  // Aplicar configuração
  meter.setConfig(config);
  
  // Inicializar BL0940
  if (!meter.begin()) {
    Serial.println("Erro ao inicializar BL0940!");
    while(1) delay(10);
  }
  
  // Configurar modo 60Hz (Brasil)
  meter.setFrequency(60);
  
  // Configurar taxa de atualização
  meter.setUpdateRate(400);
  
  Serial.println("BL0940 iniciado com sucesso!\n");
  
  // Mostrar configuração atual
  const BL0940Config& currentConfig = meter.getConfig();
  Serial.println("Configuração atual:");
  Serial.printf("Vref: %.3f V\n", currentConfig.Vref);
  Serial.printf("R29: %.1f Ohm\n", currentConfig.R29);
  Serial.printf("R30: %.1f Ohm\n", currentConfig.R30);
  Serial.printf("Rt: %.1f\n", currentConfig.Rt);
  Serial.printf("R2-R20: %.1f Ohm\n", currentConfig.R2);
  Serial.println();
}

void loop() {
  float voltage, current, power, energy, pf, temp;
  
  // Ler valores
  if (meter.getVoltage(&voltage) &&
      meter.getCurrent(&current) &&
      meter.getActivePower(&power) &&
      meter.getActiveEnergy(&energy) &&
      meter.getPowerFactor(&pf) &&
      meter.getTemperature(&temp)) {
        
    // Mostrar leituras
    Serial.println("─────────────────────────────");
    Serial.printf("⚡ Tensão:    %.2f V\n", voltage);
    Serial.printf("🔌 Corrente:  %.3f A\n", current);
    Serial.printf("💡 Potência:  %.2f W\n", power);
    Serial.printf("📊 FP:        %.2f %%\n", pf);
    Serial.printf("🌡️ Temp:      %.1f °C\n", temp);
    Serial.printf("⚡ Energia:   %.3f kWh\n", energy);
  } else {
    Serial.println("Erro na leitura!");
  }
  
  delay(1000);
}