/*
 * BL0940 Energy Meter - Teste Final
 * Baseado na implementação de Breno Diniz Trevisan
 */

#include <Arduino.h>
#include "MCM_BL0940.h"

// Definir pinos
#define SEL_PIN  5
#define SCK_PIN  18
#define MISO_PIN 19
#define MOSI_PIN 23

// Criar objeto
BL0940 meter(SEL_PIN, SCK_PIN, MISO_PIN, MOSI_PIN);

void setup() {
  Serial.begin(115200);
  delay(3000);
  
  Serial.println("\n╔════════════════════════════════════╗");
  Serial.println("║   BL0940 SPI - TESTE FINAL        ║");
  Serial.println("╚════════════════════════════════════╝\n");
  
  // Inicializar
  if(!meter.begin(400000)) {
    Serial.println("✗ Falha na inicialização!");
    while(1) delay(1000);
  }
  
  // Configurar
  meter.setFrequency(60);
  delay(50);
  meter.setUpdateRate(400);
  
  Serial.println("\n→ Aguardando 3s...\n");
  delay(3000);
}

void loop() {
  unsigned long loopStart = micros();
  
  float voltage = 0;
  float current = 0;
  float power = 0;
  float pf = 0;
  
  Serial.println("════════════════════════════════════");
  Serial.printf("⏱ Tempo: %lu s\n", millis() / 1000);
  Serial.println("────────────────────────────────────");
  
  unsigned long t1 = micros();
  if(meter.getVoltage(&voltage)) {
    unsigned long dt = micros() - t1;
    Serial.printf("⚡ Tensão:      %7.2f V ", voltage);
    if(voltage > 200 && voltage < 240) {
      Serial.printf("✓ (%lu µs)\n", dt);
    } else {
      Serial.printf("⚠ (%lu µs)\n", dt);
    }
  } else {
    Serial.println("⚡ Tensão:      ✗ ERRO");
  }
  
  t1 = micros();
  if(meter.getCurrent(&current)) {
    unsigned long dt = micros() - t1;
    Serial.printf("🔌 Corrente:    %7.3f A ✓ (%lu µs)\n", current, dt);
  } else {
    Serial.println("🔌 Corrente:    ✗ ERRO");
  }
  
  t1 = micros();
  if(meter.getActivePower(&power)) {
    unsigned long dt = micros() - t1;
    Serial.printf("💡 Potência:    %7.2f W ✓ (%lu µs)\n", power, dt);
  } else {
    Serial.println("💡 Potência:    ✗ ERRO");
  }
  
  t1 = micros();
  if(meter.getPowerFactor(&pf)) {
    unsigned long dt = micros() - t1;
    Serial.printf("📊 FP:          %7.2f %% ", pf);
    if(pf > 0 && pf <= 100) {
      Serial.printf("✓ (%lu µs)\n", dt);
    } else {
      Serial.printf("⚠ FORA DO RANGE (%lu µs)\n", dt);
    }
  } else {
    Serial.println("📊 FP:          ✗ ERRO");
  }
  
  unsigned long loopTotal = micros() - loopStart;
  Serial.println("────────────────────────────────────");
  Serial.printf("⏲  TOTAL: %lu µs (%.2f ms)\n", loopTotal, loopTotal / 1000.0);
  Serial.println("════════════════════════════════════\n");
  
  delay(2000);
}