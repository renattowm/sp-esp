/*
 * BL0940 Usage Example - Best Practices
 * 
 * Demonstra:
 * 1. Calibração ajustável
 * 2. Filtro de média móvel na APLICAÇÃO (não na biblioteca)
 * 3. Código limpo e organizado
 */

#include "MCM_BL0940.h"

// Declarar referência à configuração externa
extern BL0940Config myConfig;

// Pinos SPI
#define SEL_PIN   5
#define SCK_PIN   18
#define MISO_PIN  19
#define MOSI_PIN  23

// Instância do BL0940
BL0940 meter(SEL_PIN, SCK_PIN, MISO_PIN, MOSI_PIN);

// ========== FILTRO DE MÉDIA MÓVEL (aplicação do usuário) ==========

class MovingAverage {
private:
  float *buffer;
  uint8_t size;
  uint8_t index;
  bool filled;
  
public:
  MovingAverage(uint8_t bufferSize) {
    size = bufferSize;
    buffer = new float[size];
    index = 0;
    filled = false;
    for(uint8_t i = 0; i < size; i++) buffer[i] = 0;
  }
  
  ~MovingAverage() {
    delete[] buffer;
  }
  
  float add(float value) {
    buffer[index] = value;
    index = (index + 1) % size;
    if (index == 0) filled = true;
    
    float sum = 0;
    uint8_t count = filled ? size : index;
    for (uint8_t i = 0; i < count; i++) {
      sum += buffer[i];
    }
    return sum / count;
  }
};

// Filtros (5 leituras = suave, 3 leituras = responsivo)
MovingAverage voltageFilter(3);
MovingAverage currentFilter(3);
MovingAverage powerFilter(3);

// ========== SETUP ==========

void setup()
{
  Serial.begin(115200);
  Serial.println("\n=== BL0940 Example ===\n");
  
  // Aplicar configuração de hardware
  meter.setConfig(myConfig);
  
  // Verificar se config foi aplicada
  const BL0940Config& cfg = meter.getConfig();
  Serial.println("Configuração do Hardware:");
  Serial.printf("→ Vref: %.3f V\n", cfg.Vref);
  Serial.printf("→ R29:  %.1f Ohm (shunt)\n", cfg.R29);
  Serial.printf("→ R30:  %.1f Ohm\n", cfg.R30);
  Serial.printf("→ Rt:   %.1f (range)\n", cfg.Rt);
  Serial.printf("→ R2-R20: %.0f Ohm (divisor tensão)\n", cfg.R2);
  Serial.println();
  
  if (!meter.begin()) {
    Serial.println("Falha ao inicializar BL0940!");
    while(1);
  }
  
  Serial.println("BL0940 inicializado!");
  
  // Configurações básicas
  meter.setFrequency(60);      // 50Hz ou 60Hz
  meter.setUpdateRate(400);    // 400ms ou 800ms
  
  // ========== CALIBRAÇÃO ==========
  // Ajustar baseado em medições reais:
  
  // Para cargas BAIXAS (< 1A):
  // meter.setCurrentCalibration(1.05);  // +5%
  // meter.setPowerCalibration(1.05);
  
  // Para cargas MÉDIAS (1-5A):
  meter.setCurrentCalibration(1.00);  // sem correção
  meter.setPowerCalibration(1.00);
  
  // Para cargas ALTAS (> 5A):
  // meter.setCurrentCalibration(0.97);  // -3%
  // meter.setPowerCalibration(0.97);
  
  Serial.println("→ Calibração configurada");
  Serial.println("→ Aguardando estabilização...\n");
  delay(2000);
}

// ========== LOOP ==========

void loop()
{
  float voltage, current, power, pf, temp, energy;
  
  // Ler valores RAW (sem filtro)
  if (meter.getVoltage(&voltage)) {
    voltage = voltageFilter.add(voltage);  // Aplicar filtro
    Serial.printf("⚡ Tensão:  %6.2f V\n", voltage);
  }
  
  if (meter.getCurrent(&current)) {
    current = currentFilter.add(current);
    Serial.printf("🔌 Corrente: %6.3f A\n", current);
  }
  
  if (meter.getActivePower(&power)) {
    power = powerFilter.add(power);
    Serial.printf("💡 Potência: %6.2f W\n", power);
  }
  
  if (meter.getPowerFactor(&pf)) {
    Serial.printf("📊 FP:       %6.2f %%\n", pf);
  }
  
  if (meter.getTemperature(&temp)) {
    Serial.printf("🌡️  Temp:     %6.1f °C\n", temp);
  }
  
  if (meter.getActiveEnergy(&energy)) {
    Serial.printf("⚡ Energia:  %6.3f kWh\n", energy);
  }
  
  // Cálculo manual de verificação
  float calculated_power = voltage * current;
  Serial.printf("\n✓ V×I = %.2f W (calculado)\n", calculated_power);
  Serial.println("───────────────────────────\n");
  
  delay(2000);
}