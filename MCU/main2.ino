/**
 * @file main2.ino
 * @brief Simulador de memoria RAM para ESP32 vía SPI Slave
 * 
 * Este programa simula una memoria RAM que puede ser leída por otro ESP32
 * a través de comunicación SPI. Es útil para probar el sistema sin necesidad
 * de memoria RAM física.
 * 
 * Funcionalidades:
 * - Actúa como SPI Slave
 * - Simula memoria RAM de 64KB (65536 bytes)
 * - Responde a comandos de lectura (0x03)
 * - Genera datos de prueba (ondas sinusoidales, diente de sierra, etc.)
 * 
 * Conexiones SPI requeridas:
 * - MOSI: GPIO 23 (datos del maestro al esclavo)
 * - MISO: GPIO 19 (datos del esclavo al maestro) 
 * - SCK:  GPIO 18 (reloj)
 * - CS:   GPIO 5  (selección del esclavo)
 * 
 * @author ESP32 Assistant
 * @date 2024
 */

#include <SPI.h>

// Configuración de la memoria simulada
const uint32_t RAM_SIZE = 65536;        // 64KB de memoria simulada
const uint8_t CMD_READ = 0x03;          // Comando de lectura estándar
const uint8_t ADDR_BYTES = 3;           // 24 bits de dirección

// Buffer para simular la memoria RAM
uint8_t simulatedRAM[RAM_SIZE];

// Estado del SPI slave
volatile bool dataRequested = false;
volatile uint32_t currentAddress = 0;
volatile uint8_t addressBytesReceived = 0;
volatile uint8_t command = 0;
volatile bool commandReceived = false;

/**
 * @brief Genera datos de prueba en la memoria simulada
 * 
 * Crea diferentes patrones de datos que son útiles para probar
 * el sistema de adquisición y procesamiento:
 * - Onda sinusoidal (0-8191)
 * - Onda diente de sierra (8192-16383)
 * - Onda cuadrada (16384-24575)
 * - Ruido pseudo-aleatorio (24576-32767)
 * - Rampa lineal (32768-40959)
 * - Datos constantes (40960-49151)
 * - Patrón de prueba (49152-57343)
 * - Reservado para expansión (57344-65535)
 */
void generateTestData() {
  Serial.println("Generando datos de prueba...");
  
  for (uint32_t i = 0; i < RAM_SIZE; i++) {
    if (i < 8192) {
      // Onda sinusoidal (0-8191)
      float angle = (2.0 * PI * i) / 100.0;  // 100 muestras por ciclo
      simulatedRAM[i] = 128 + (uint8_t)(127.0 * sin(angle));
      
    } else if (i < 16384) {
      // Onda diente de sierra (8192-16383)
      uint32_t pos = i - 8192;
      simulatedRAM[i] = (pos % 256);
      
    } else if (i < 24576) {
      // Onda cuadrada (16384-24575)
      uint32_t pos = i - 16384;
      simulatedRAM[i] = ((pos / 128) % 2) ? 200 : 55;
      
    } else if (i < 32768) {
      // Ruido pseudo-aleatorio (24576-32767)
      simulatedRAM[i] = (uint8_t)(random(0, 256));
      
    } else if (i < 40960) {
      // Rampa lineal (32768-40959)
      uint32_t pos = i - 32768;
      simulatedRAM[i] = (pos * 255) / 8191;
      
    } else if (i < 49152) {
      // Datos constantes para pruebas de trigger (40960-49151)
      if ((i - 40960) < 2000) {
        simulatedRAM[i] = 100;  // Nivel bajo
      } else if ((i - 40960) < 4000) {
        simulatedRAM[i] = 180;  // Nivel alto (trigger aquí)
      } else {
        simulatedRAM[i] = 100;  // Vuelta al nivel bajo
      }
      
    } else if (i < 57344) {
      // Patrón de prueba específico (49152-57343)
      uint32_t pos = (i - 49152) % 16;
      const uint8_t pattern[] = {0, 32, 64, 96, 128, 160, 192, 224, 255, 224, 192, 160, 128, 96, 64, 32};
      simulatedRAM[i] = pattern[pos];
      
    } else {
      // Área reservada (57344-65535)
      simulatedRAM[i] = 0xFF;
    }
  }
  
  Serial.printf("Datos de prueba generados para %d bytes\n", RAM_SIZE);
  Serial.println("Sectores de memoria:");
  Serial.println("  0x0000-0x1FFF: Onda sinusoidal");
  Serial.println("  0x2000-0x3FFF: Diente de sierra");
  Serial.println("  0x4000-0x5FFF: Onda cuadrada");
  Serial.println("  0x6000-0x7FFF: Ruido aleatorio");
  Serial.println("  0x8000-0x9FFF: Rampa lineal");
  Serial.println("  0xA000-0xBFFF: Datos con trigger");
  Serial.println("  0xC000-0xDFFF: Patrón de prueba");
  Serial.println("  0xE000-0xFFFF: Area reservada");
}

/**
 * @brief Rutina de interrupción SPI - maneja la comunicación como slave
 * 
 * Esta función se ejecuta cada vez que el maestro envía o solicita datos.
 * Maneja el protocolo de comandos compatible con memorias SPI estándar.
 */
void IRAM_ATTR spiSlaveISR() {
  // Leer byte recibido del maestro
  uint8_t receivedByte = SPI2.slave.rd_buf[0];
  
  if (!commandReceived) {
    // Primer byte es el comando
    command = receivedByte;
    commandReceived = true;
    addressBytesReceived = 0;
    currentAddress = 0;
    
  } else if (command == CMD_READ && addressBytesReceived < ADDR_BYTES) {
    // Recibiendo bytes de dirección (24 bits, MSB primero)
    currentAddress = (currentAddress << 8) | receivedByte;
    addressBytesReceived++;
    
    if (addressBytesReceived == ADDR_BYTES) {
      // Dirección completa recibida, preparar datos
      dataRequested = true;
      // Verificar que la dirección esté en rango
      if (currentAddress >= RAM_SIZE) {
        currentAddress = 0;
      }
    }
    
  } else if (command == CMD_READ && dataRequested) {
    // Enviar datos de la memoria simulada
    SPI2.slave.wr_buf[0] = simulatedRAM[currentAddress];
    currentAddress = (currentAddress + 1) % RAM_SIZE; // Wrap around
  }
  
  // Limpiar flags de SPI
  SPI2.slave.trans_done = 0;
  SPI2.slave.rd_buf_done = 0;
  SPI2.slave.wr_buf_done = 0;
}

/**
 * @brief Configuración inicial del sistema
 */
void setup() {
  // Inicializar comunicación serie para debug
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  Serial.println("\n=== ESP32 RAM Simulator ===");
  Serial.println("Configurando SPI Slave...");
  
  // Generar datos de prueba
  generateTestData();
  
  // Configurar pines SPI
  pinMode(MISO, OUTPUT);  // GPIO 19
  pinMode(MOSI, INPUT);   // GPIO 23  
  pinMode(SCK, INPUT);    // GPIO 18
  pinMode(SS, INPUT);     // GPIO 5 (CS)
  
  // Configurar SPI2 como slave
  SPI2.slave.addr = 0;
  SPI2.slave.mode = 0;                    // SPI Mode 0
  SPI2.slave.miso_dlen.usr_miso_dbitlen = 7;  // 8 bits
  SPI2.slave.mosi_dlen.usr_mosi_dbitlen = 7;  // 8 bits
  SPI2.slave.rd_byte_order = 0;           // MSB first
  SPI2.slave.wr_byte_order = 0;           // MSB first
  
  // Habilitar interrupciones SPI
  SPI2.slave.trans_inten = 1;
  SPI2.slave.rd_buf_inten = 1;
  
  // Registrar rutina de interrupción
  attachInterrupt(digitalPinToInterrupt(SS), spiSlaveISR, FALLING);
  
  // Inicializar estado
  commandReceived = false;
  dataRequested = false;
  currentAddress = 0;
  addressBytesReceived = 0;
  
  Serial.println("SPI Slave configurado correctamente");
  Serial.println("Esperando comandos del maestro...");
  Serial.println("Conexiones:");
  Serial.println("  MOSI -> GPIO 23");
  Serial.println("  MISO -> GPIO 19");
  Serial.println("  SCK  -> GPIO 18");
  Serial.println("  CS   -> GPIO 5");
  Serial.println();
}

/**
 * @brief Bucle principal - monitoreo y estadísticas
 */
void loop() {
  static unsigned long lastStatsTime = 0;
  static uint32_t transactionCount = 0;
  static uint32_t lastAddress = 0;
  
  // Mostrar estadísticas cada 5 segundos
  if (millis() - lastStatsTime > 5000) {
    lastStatsTime = millis();
    
    Serial.println("=== Estadísticas del Simulador ===");
    Serial.printf("Tiempo activo: %lu ms\n", millis());
    Serial.printf("Última dirección accedida: 0x%06X\n", lastAddress);
    Serial.printf("Comando actual: 0x%02X\n", command);
    Serial.printf("Estado: %s\n", dataRequested ? "Enviando datos" : "Esperando comando");
    
    // Mostrar muestra de datos actuales
    Serial.print("Datos en dirección actual: ");
    for (int i = 0; i < 16 && (currentAddress + i) < RAM_SIZE; i++) {
      Serial.printf("%02X ", simulatedRAM[currentAddress + i]);
    }
    Serial.println();
    Serial.println();
  }
  
  // Actualizar estadísticas si hubo actividad
  if (currentAddress != lastAddress) {
    lastAddress = currentAddress;
    transactionCount++;
  }
  
  // Reset periódico del estado en caso de problemas de comunicación
  static unsigned long lastResetTime = 0;
  if (millis() - lastResetTime > 10000) { // Reset cada 10 segundos si no hay actividad
    if (!dataRequested && commandReceived) {
      commandReceived = false;
      Serial.println("Reset de estado por inactividad");
    }
    lastResetTime = millis();
  }
  
  // Pequeña pausa para no sobrecargar el CPU
  delay(100);
}