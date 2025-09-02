/**
 * @file MCU.ino
 * @brief Sistema principal de adquisición y procesamiento de datos
 * 
 * Este programa integra el lector de RAM via SPI con el sistema PDS
 * para crear un osciloscopio digital que procesa señales y genera
 * matrices de 800x600 para visualización.
 * 
 * Componentes principales:
 * - RamReader: Lee datos de memoria RAM via SPI
 * - PDS: Procesa datos con funcionalidad de osciloscopio
 * - Otros módulos: UART, Display, Peripherals, Net (stub)
 * 
 * @author ESP32 Assistant  
 * @date 2024
 */

#include "ram_reader/ram_reader.cpp"
#include "uart_comm/uart_comm.cpp"
#include "display/display.cpp"
#include "peripherals/peripherals.cpp"
#include "pds/pds.cpp"
#include "net/net.cpp" 

// Instancias de los módulos principales
RamReader ram(5); // CS en GPIO 5
UartComm uartComm;
Display display;
Peripherals peripherals;
Pds pds;
Net net;

// Buffer para datos leídos de RAM
const size_t SAMPLE_BUFFER_SIZE = 1024;
uint8_t sampleBuffer[SAMPLE_BUFFER_SIZE];

// Matriz de salida para visualización (800x600)
uint8_t displayMatrix[Pds::MATRIX_HEIGHT][Pds::MATRIX_WIDTH];

// Variables de configuración
uint32_t currentRAMAddress = 0x0000;  // Dirección inicial en RAM
bool continuousMode = true;           // Modo de operación continua
unsigned long lastCaptureTime = 0;
const unsigned long CAPTURE_INTERVAL = 1000; // Captura cada 1000ms

/**
 * @brief Configuración inicial del sistema
 */
void setup() {
  // Inicializar comunicación serie para debug
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  Serial.println("\n=== Sistema de Osciloscopio ESP32 ===");
  Serial.println("Inicializando módulos...");
  
  // Inicializar módulos principales
  ram.begin();
  pds.begin();
  uartComm.begin();
  display.begin();
  peripherals.begin();
  net.begin();
  
  // Configurar parámetros del PDS (osciloscopio)
  pds.setTrigger(Pds::TRIGGER_RISING, 128);  // Trigger en flanco ascendente, nivel 128
  pds.setAmplitudeScale(1.0f);               // Sin escalado de amplitud
  pds.setTimeScale(1);                       // Una muestra por píxel
  
  Serial.println("Sistema inicializado correctamente");
  Serial.println("\nComandos disponibles por Serial:");
  Serial.println("  's' - Captura única (single shot)");
  Serial.println("  'c' - Modo continuo");
  Serial.println("  't' - Cambiar tipo de trigger");
  Serial.println("  'a' - Cambiar dirección de RAM");
  Serial.println("  '+' - Aumentar escala de amplitud");
  Serial.println("  '-' - Disminuir escala de amplitud");
  Serial.println("  'h' - Mostrar esta ayuda");
  Serial.println();
}

/**
 * @brief Bucle principal del sistema
 */
void loop() {
  // Procesar comandos del usuario
  processSerialCommands();
  
  // Verificar si es tiempo de capturar datos
  if (continuousMode && (millis() - lastCaptureTime >= CAPTURE_INTERVAL)) {
    performDataCapture();
    lastCaptureTime = millis();
  }
  
  // Pequeña pausa para no sobrecargar el sistema
  delay(50);
}

/**
 * @brief Realiza la captura y procesamiento de datos
 */
void performDataCapture() {
  if (!ram.isReady()) {
    Serial.println("Error: RamReader no está listo");
    return;
  }
  
  Serial.printf("Capturando %d bytes desde dirección 0x%06X...\n", 
                SAMPLE_BUFFER_SIZE, currentRAMAddress);
  
  // Leer datos de la RAM
  ram.readBlock(currentRAMAddress, sampleBuffer, SAMPLE_BUFFER_SIZE);
  
  // Mostrar muestra de los datos leídos
  Serial.print("Primeros 16 bytes: ");
  for (int i = 0; i < 16; i++) {
    if (sampleBuffer[i] < 16) Serial.print('0');
    Serial.print(sampleBuffer[i], HEX);
    Serial.print(' ');
  }
  Serial.println();
  
  // Procesar datos con PDS
  bool success = pds.processData(sampleBuffer, SAMPLE_BUFFER_SIZE, displayMatrix);
  
  if (success) {
    Serial.printf("PDS: Datos procesados exitosamente\n");
    
    // Mostrar información del trigger
    if (pds.getTriggerStatus()) {
      Serial.printf("PDS: Trigger detectado en posición %zu\n", pds.getTriggerPosition());
    } else {
      Serial.println("PDS: Sin trigger detectado");
    }
    
    // Mostrar estadísticas de la matriz generada
    showMatrixStats();
    
  } else {
    Serial.println("PDS: Error al procesar datos");
  }
}

/**
 * @brief Muestra estadísticas de la matriz de salida
 */
void showMatrixStats() {
  uint32_t pixelsDrawn = 0;
  uint32_t maxValue = 0;
  uint32_t minValue = 255;
  
  // Contar píxeles dibujados y encontrar valores min/max
  for (uint16_t y = 0; y < Pds::MATRIX_HEIGHT; y++) {
    for (uint16_t x = 0; x < Pds::MATRIX_WIDTH; x++) {
      uint8_t pixelValue = displayMatrix[y][x];
      if (pixelValue > 0) {
        pixelsDrawn++;
        if (pixelValue > maxValue) maxValue = pixelValue;
        if (pixelValue < minValue) minValue = pixelValue;
      }
    }
  }
  
  Serial.printf("Matriz generada: %lu píxeles dibujados (valor: %lu-%lu)\n", 
                pixelsDrawn, minValue, maxValue);
  
  // Mostrar una línea horizontal de muestra (línea central)
  uint16_t sampleY = Pds::MATRIX_HEIGHT / 2;
  Serial.printf("Muestra línea Y=%d: ", sampleY);
  for (uint16_t x = 0; x < ((80 < Pds::MATRIX_WIDTH) ? 80 : Pds::MATRIX_WIDTH); x += 10) {
    Serial.printf("%02X ", displayMatrix[sampleY][x]);
  }
  Serial.println();
}

/**
 * @brief Procesa comandos recibidos por el puerto serie
 */
void processSerialCommands() {
  if (Serial.available()) {
    char cmd = Serial.read();
    
    switch (cmd) {
      case 's':
      case 'S':
        // Captura única
        continuousMode = false;
        Serial.println("Modo: Captura única");
        performDataCapture();
        break;
        
      case 'c':
      case 'C':
        // Modo continuo
        continuousMode = true;
        Serial.println("Modo: Continuo activado");
        break;
        
      case 't':
      case 'T':
        // Cambiar tipo de trigger
        cycleTriggerMode();
        break;
        
      case 'a':
      case 'A':
        // Cambiar dirección de RAM
        changeRAMAddress();
        break;
        
      case '+':
        // Aumentar escala de amplitud
        adjustAmplitudeScale(1.25f);
        break;
        
      case '-':
        // Disminuir escala de amplitud  
        adjustAmplitudeScale(0.8f);
        break;
        
      case 'h':
      case 'H':
        // Mostrar ayuda
        showHelp();
        break;
        
      default:
        Serial.printf("Comando desconocido: '%c'. Usa 'h' para ayuda.\n", cmd);
        break;
    }
  }
}

/**
 * @brief Cambia el modo de trigger de forma cíclica
 */
void cycleTriggerMode() {
  static Pds::TriggerMode currentMode = Pds::TRIGGER_RISING;
  
  switch (currentMode) {
    case Pds::TRIGGER_OFF:
      currentMode = Pds::TRIGGER_RISING;
      pds.setTrigger(currentMode, 128);
      Serial.println("Trigger: Flanco ascendente");
      break;
      
    case Pds::TRIGGER_RISING:
      currentMode = Pds::TRIGGER_FALLING;
      pds.setTrigger(currentMode, 128);
      Serial.println("Trigger: Flanco descendente");
      break;
      
    case Pds::TRIGGER_FALLING:
      currentMode = Pds::TRIGGER_LEVEL;
      pds.setTrigger(currentMode, 128);
      Serial.println("Trigger: Nivel");
      break;
      
    case Pds::TRIGGER_LEVEL:
      currentMode = Pds::TRIGGER_OFF;
      pds.setTrigger(currentMode, 128);
      Serial.println("Trigger: Desactivado");
      break;
  }
}

/**
 * @brief Cambia la dirección de inicio en RAM
 */
void changeRAMAddress() {
  // Ciclar entre diferentes secciones de memoria del simulador
  const uint32_t addresses[] = {
    0x0000,   // Onda sinusoidal
    0x2000,   // Diente de sierra
    0x4000,   // Onda cuadrada
    0x6000,   // Ruido aleatorio
    0x8000,   // Rampa lineal
    0xA000,   // Datos con trigger
    0xC000    // Patrón de prueba
  };
  
  static size_t addressIndex = 0;
  
  addressIndex = (addressIndex + 1) % (sizeof(addresses) / sizeof(addresses[0]));
  currentRAMAddress = addresses[addressIndex];
  
  const char* sectionNames[] = {
    "Onda sinusoidal",
    "Diente de sierra", 
    "Onda cuadrada",
    "Ruido aleatorio",
    "Rampa lineal",
    "Datos con trigger",
    "Patrón de prueba"
  };
  
  Serial.printf("Dirección RAM: 0x%06X (%s)\n", 
                currentRAMAddress, sectionNames[addressIndex]);
}

/**
 * @brief Ajusta la escala de amplitud
 * @param factor Factor multiplicativo para la escala actual
 */
void adjustAmplitudeScale(float factor) {
  static float currentScale = 1.0f;
  
  currentScale *= factor;
  
  // Limitar rango razonable
  if (currentScale < 0.1f) currentScale = 0.1f;
  if (currentScale > 10.0f) currentScale = 10.0f;
  
  pds.setAmplitudeScale(currentScale);
  Serial.printf("Escala de amplitud: %.2f\n", currentScale);
}

/**
 * @brief Muestra la ayuda de comandos
 */
void showHelp() {
  Serial.println("\n=== Comandos Disponibles ===");
  Serial.println("  's' - Captura única (single shot)");
  Serial.println("  'c' - Modo continuo");
  Serial.println("  't' - Cambiar tipo de trigger (OFF->RISING->FALLING->LEVEL->OFF)");
  Serial.println("  'a' - Cambiar dirección de RAM (cicla entre secciones)");
  Serial.println("  '+' - Aumentar escala de amplitud x1.25");
  Serial.println("  '-' - Disminuir escala de amplitud x0.8");
  Serial.println("  'h' - Mostrar esta ayuda");
  Serial.println("\n=== Estado Actual ===");
  Serial.printf("Dirección RAM: 0x%06X\n", currentRAMAddress);
  Serial.printf("Modo: %s\n", continuousMode ? "Continuo" : "Captura única");
  Serial.printf("Trigger: %s\n", pds.getTriggerStatus() ? "Activo" : "Inactivo");
  Serial.println();
}
