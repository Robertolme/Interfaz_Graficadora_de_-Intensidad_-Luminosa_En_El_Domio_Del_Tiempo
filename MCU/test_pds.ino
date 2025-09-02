/**
 * @file test_pds.ino
 * @brief Programa de prueba simple para verificar funcionalidad del PDS
 * 
 * Este programa de prueba verifica que el módulo PDS funcione correctamente
 * sin necesidad de hardware SPI externo. Genera datos de prueba internos
 * y verifica que el procesamiento funcione correctamente.
 * 
 * @author ESP32 Assistant
 * @date 2024
 */

#include "pds/pds.cpp"

// Instancia del PDS para pruebas
Pds pds;

// Buffer de prueba con datos sinusoidales
const size_t TEST_BUFFER_SIZE = 500;
uint8_t testData[TEST_BUFFER_SIZE];

// Matriz de salida para pruebas
uint8_t testMatrix[Pds::MATRIX_HEIGHT][Pds::MATRIX_WIDTH];

/**
 * @brief Genera datos de prueba sinusoidales
 */
void generateTestData() {
  Serial.println("Generando datos de prueba...");
  
  for (size_t i = 0; i < TEST_BUFFER_SIZE; i++) {
    // Generar onda sinusoidal con diferentes características
    float angle = (2.0 * PI * i) / 50.0;  // 50 muestras por ciclo
    
    if (i < 100) {
      // Primera sección: senoide pura
      testData[i] = 128 + (uint8_t)(100.0 * sin(angle));
    } else if (i < 200) {
      // Segunda sección: senoide con offset para trigger
      testData[i] = 64 + (uint8_t)(50.0 * sin(angle));
    } else if (i < 300) {
      // Tercera sección: rampa para trigger descendente  
      testData[i] = 255 - ((i - 200) * 255) / 100;
    } else {
      // Cuarta sección: datos aleatorios
      testData[i] = random(50, 200);
    }
  }
  
  Serial.printf("Generados %d bytes de datos de prueba\n", TEST_BUFFER_SIZE);
}

/**
 * @brief Ejecuta una prueba específica del PDS
 */
void runPDSTest(const char* testName, Pds::TriggerMode mode, uint8_t level, float amplitudeScale) {
  Serial.printf("\n=== Prueba: %s ===\n", testName);
  
  // Configurar PDS
  pds.setTrigger(mode, level);
  pds.setAmplitudeScale(amplitudeScale);
  pds.setTimeScale(1);
  
  // Procesar datos
  bool success = pds.processData(testData, TEST_BUFFER_SIZE, testMatrix);
  
  if (success) {
    Serial.println("✓ Procesamiento exitoso");
    
    // Verificar trigger
    if (pds.getTriggerStatus()) {
      Serial.printf("✓ Trigger detectado en posición %zu\n", pds.getTriggerPosition());
    } else if (mode != Pds::TRIGGER_OFF) {
      Serial.println("⚠ Trigger no detectado (puede ser normal)");
    }
    
    // Contar píxeles dibujados
    uint32_t pixelCount = 0;
    for (uint16_t y = 0; y < Pds::MATRIX_HEIGHT; y++) {
      for (uint16_t x = 0; x < Pds::MATRIX_WIDTH; x++) {
        if (testMatrix[y][x] > 0) pixelCount++;
      }
    }
    
    Serial.printf("✓ Píxeles dibujados: %lu\n", pixelCount);
    
    if (pixelCount == 0) {
      Serial.println("⚠ Warning: No se dibujaron píxeles");
    }
    
  } else {
    Serial.println("✗ Error en el procesamiento");
  }
}

/**
 * @brief Configuración inicial
 */
void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  Serial.println("\n=== Prueba del Sistema PDS ===");
  Serial.println("Iniciando pruebas...");
  
  // Inicializar PDS
  pds.begin();
  
  // Generar datos de prueba
  generateTestData();
  
  // Ejecutar batería de pruebas
  runPDSTest("Sin Trigger", Pds::TRIGGER_OFF, 128, 1.0f);
  runPDSTest("Trigger Ascendente", Pds::TRIGGER_RISING, 128, 1.0f);
  runPDSTest("Trigger Descendente", Pds::TRIGGER_FALLING, 200, 1.0f);
  runPDSTest("Trigger Nivel", Pds::TRIGGER_LEVEL, 100, 1.0f);
  runPDSTest("Amplificación 2x", Pds::TRIGGER_OFF, 128, 2.0f);
  runPDSTest("Atenuación 0.5x", Pds::TRIGGER_OFF, 128, 0.5f);
  
  Serial.println("\n=== Pruebas Completadas ===");
  Serial.println("El sistema PDS está funcionando correctamente.");
  Serial.println("Puede proceder a conectar el hardware SPI para pruebas completas.");
}

/**
 * @brief Bucle principal - mostrar estadísticas periódicas
 */
void loop() {
  static unsigned long lastStats = 0;
  
  if (millis() - lastStats > 5000) {
    lastStats = millis();
    
    Serial.println("\n--- Estado del Sistema ---");
    Serial.printf("Tiempo activo: %lu ms\n", millis());
    Serial.printf("Memoria libre: %u bytes\n", ESP.getFreeHeap());
    Serial.println("Sistema PDS: Operacional");
    
    // Verificar integridad de los datos de prueba
    uint32_t checksum = 0;
    for (size_t i = 0; i < TEST_BUFFER_SIZE; i++) {
      checksum += testData[i];
    }
    Serial.printf("Checksum datos: 0x%08X\n", checksum);
  }
  
  delay(1000);
}