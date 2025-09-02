/**
 * @file sensor_example.ino
 * @brief Ejemplo de uso con sensores reales
 * 
 * Este ejemplo muestra cómo modificar el simulador de RAM (main2.ino)
 * para capturar datos reales de sensores en lugar de generar datos de prueba.
 * 
 * Sensores compatibles:
 * - Sensores analógicos (0-3.3V) vía ADC
 * - Sensores digitales vía GPIO
 * - Sensores I2C (acelerómetros, giroscopios, etc.)
 * 
 * @author ESP32 Assistant
 * @date 2024
 */

// Incluir librerías según el sensor usado
// #include <Wire.h>           // Para sensores I2C
// #include <OneWire.h>        // Para sensores 1-Wire
// #include <DHT.h>            // Para sensor de temperatura/humedad

#include <SPI.h>

// ========== CONFIGURACIÓN DE SENSORES ==========

// Ejemplo 1: Sensor analógico simple (potenciómetro, LDR, etc.)
const int ANALOG_SENSOR_PIN = A0;  // Pin analógico

// Ejemplo 2: Sensor de temperatura DS18B20
// OneWire oneWire(4);             // Pin 4
// DallasTemperature sensors(&oneWire);

// Ejemplo 3: Acelerómetro I2C  
// const int MPU6050_ADDR = 0x68;

// ========== CONFIGURACIÓN RAM SIMULADA ==========
const uint32_t RAM_SIZE = 65536;
const uint8_t CMD_READ = 0x03;
const uint8_t ADDR_BYTES = 3;

uint8_t simulatedRAM[RAM_SIZE];

// Variables para captura de datos
unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 10;  // Leer sensor cada 10ms
uint32_t writeIndex = 0;  // Índice de escritura en RAM

// Variables de control SPI (igual que main2.ino)
volatile bool dataRequested = false;
volatile uint32_t currentAddress = 0;
volatile uint8_t addressBytesReceived = 0;
volatile uint8_t command = 0;
volatile bool commandReceived = false;

/**
 * @brief Lee el valor del sensor seleccionado
 * @return Valor del sensor escalado a 0-255
 */
uint8_t readSensorValue() {
  // ===== EJEMPLO 1: SENSOR ANALÓGICO SIMPLE =====
  int analogValue = analogRead(ANALOG_SENSOR_PIN);
  return map(analogValue, 0, 4095, 0, 255);  // ESP32 ADC es de 12 bits
  
  /* 
  // ===== EJEMPLO 2: SENSOR DE TEMPERATURA DS18B20 =====
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  // Mapear temperatura (-40°C a +85°C) a 0-255
  int mappedTemp = map((int)(tempC * 10), -400, 850, 0, 255);
  return constrain(mappedTemp, 0, 255);
  */
  
  /*
  // ===== EJEMPLO 3: ACELERÓMETRO (Eje X) =====
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x3B);  // Registro ACCEL_XOUT_H
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, 2);
  
  int16_t accelX = Wire.read() << 8 | Wire.read();
  // Mapear rango del acelerómetro (-32768 a +32767) a 0-255
  return map(accelX, -32768, 32767, 0, 255);
  */
  
  /*
  // ===== EJEMPLO 4: SENSOR DIGITAL (BOTÓN/SWITCH) =====
  bool digitalState = digitalRead(2);  // Pin 2
  return digitalState ? 255 : 0;  // Alto = 255, Bajo = 0
  */
  
  /*
  // ===== EJEMPLO 5: MICRÓFONO ANALÓGICO =====
  int audioSample = analogRead(A0);
  // Aplicar filtro pasa-altos simple para AC coupling
  static int previousSample = 2048;
  int filteredSample = audioSample - previousSample + 2048;
  previousSample = audioSample;
  
  return map(constrain(filteredSample, 0, 4095), 0, 4095, 0, 255);
  */
  
  /*
  // ===== EJEMPLO 6: SENSOR DE LUZ (LDR) =====
  int lightLevel = analogRead(A0);
  // Aplicar curva logarítmica para mejor respuesta visual
  float logValue = log(lightLevel + 1) / log(4096) * 255;
  return (uint8_t)constrain(logValue, 0, 255);
  */
}

/**
 * @brief Inicializa el sensor seleccionado
 */
void initializeSensor() {
  Serial.println("Inicializando sensor...");
  
  // ===== EJEMPLO 1: SENSOR ANALÓGICO =====
  pinMode(ANALOG_SENSOR_PIN, INPUT);
  
  /*
  // ===== EJEMPLO 2: SENSOR DS18B20 =====
  sensors.begin();
  Serial.printf("Sensores DS18B20 encontrados: %d\n", sensors.getDeviceCount());
  */
  
  /*
  // ===== EJEMPLO 3: ACELERÓMETRO I2C =====
  Wire.begin();
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // Despertar el MPU-6050
  Wire.endTransmission(true);
  */
  
  Serial.println("Sensor inicializado correctamente");
}

/**
 * @brief Captura datos del sensor y los almacena en RAM simulada
 */
void captureSensorData() {
  if (millis() - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = millis();
    
    // Leer valor del sensor
    uint8_t sensorValue = readSensorValue();
    
    // Almacenar en RAM simulada (buffer circular)
    simulatedRAM[writeIndex] = sensorValue;
    writeIndex = (writeIndex + 1) % RAM_SIZE;
    
    // Debug: mostrar valor ocasionalmente
    static int debugCounter = 0;
    if (++debugCounter >= 100) {  // Cada 100 lecturas (1 segundo)
      debugCounter = 0;
      Serial.printf("Sensor: %d (0x%02X) [Pos: %lu]\n", 
                    sensorValue, sensorValue, writeIndex);
    }
  }
}

/**
 * @brief Rutina de interrupción SPI (igual que main2.ino)
 */
void IRAM_ATTR spiSlaveISR() {
  uint8_t receivedByte = SPI2.slave.rd_buf[0];
  
  if (!commandReceived) {
    command = receivedByte;
    commandReceived = true;
    addressBytesReceived = 0;
    currentAddress = 0;
    
  } else if (command == CMD_READ && addressBytesReceived < ADDR_BYTES) {
    currentAddress = (currentAddress << 8) | receivedByte;
    addressBytesReceived++;
    
    if (addressBytesReceived == ADDR_BYTES) {
      dataRequested = true;
      if (currentAddress >= RAM_SIZE) {
        currentAddress = 0;
      }
    }
    
  } else if (command == CMD_READ && dataRequested) {
    SPI2.slave.wr_buf[0] = simulatedRAM[currentAddress];
    currentAddress = (currentAddress + 1) % RAM_SIZE;
  }
  
  SPI2.slave.trans_done = 0;
  SPI2.slave.rd_buf_done = 0;
  SPI2.slave.wr_buf_done = 0;
}

/**
 * @brief Configuración inicial
 */
void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  Serial.println("\n=== ESP32 Sensor Data Capture ===");
  Serial.println("Capturando datos reales de sensores...");
  
  // Inicializar sensor
  initializeSensor();
  
  // Llenar buffer inicial con datos del sensor
  Serial.println("Llenando buffer inicial...");
  for (uint32_t i = 0; i < min(1000U, RAM_SIZE); i++) {
    simulatedRAM[i] = readSensorValue();
    delay(1);  // Pequeña pausa entre lecturas
  }
  
  // Configurar SPI como slave (igual que main2.ino)
  pinMode(MISO, OUTPUT);  
  pinMode(MOSI, INPUT);   
  pinMode(SCK, INPUT);    
  pinMode(SS, INPUT);     
  
  SPI2.slave.addr = 0;
  SPI2.slave.mode = 0;
  SPI2.slave.miso_dlen.usr_miso_dbitlen = 7;
  SPI2.slave.mosi_dlen.usr_mosi_dbitlen = 7;
  SPI2.slave.rd_byte_order = 0;
  SPI2.slave.wr_byte_order = 0;
  SPI2.slave.trans_inten = 1;
  SPI2.slave.rd_buf_inten = 1;
  
  attachInterrupt(digitalPinToInterrupt(SS), spiSlaveISR, FALLING);
  
  commandReceived = false;
  dataRequested = false;
  currentAddress = 0;
  addressBytesReceived = 0;
  
  Serial.println("Sistema listo - Capturando datos del sensor");
  Serial.println("Conectar ESP32 osciloscopio para visualizar");
}

/**
 * @brief Bucle principal
 */
void loop() {
  // Capturar datos del sensor continuamente
  captureSensorData();
  
  // Estadísticas cada 5 segundos
  static unsigned long lastStats = 0;
  if (millis() - lastStats > 5000) {
    lastStats = millis();
    
    Serial.println("\n=== Estadísticas ===");
    Serial.printf("Muestras capturadas: %lu\n", writeIndex);
    Serial.printf("Memoria usada: %.1f%%\n", (float)writeIndex / RAM_SIZE * 100.0);
    Serial.printf("Última dirección SPI: 0x%06X\n", currentAddress);
    
    // Mostrar últimos valores capturados
    Serial.print("Últimos valores: ");
    uint32_t startIdx = (writeIndex >= 10) ? writeIndex - 10 : 0;
    for (uint32_t i = startIdx; i < writeIndex && i < startIdx + 10; i++) {
      Serial.printf("%02X ", simulatedRAM[i]);
    }
    Serial.println();
  }
  
  // Pequeña pausa para no sobrecargar el sistema
  delay(1);
}

// ========== NOTAS DE USO ==========
/*

INSTRUCCIONES DE USO:

1. Seleccionar el sensor que quieres usar:
   - Descomentar el código del sensor deseado en readSensorValue()
   - Comentar los ejemplos que no uses
   - Agregar las librerías necesarias al inicio del archivo

2. Conectar el sensor:
   - Sensor analógico: Conectar a pin A0 (GPIO 36)
   - Sensor digital: Conectar a pin configurado en el código
   - Sensores I2C: SDA=21, SCL=22 en la mayoría de ESP32

3. Cargar este programa en un ESP32 (simulador/capturador)

4. Cargar MCU.ino en otro ESP32 (osciloscopio)

5. Conectar ambos ESP32 via SPI según SETUP.md

6. Usar los comandos del osciloscopio para visualizar datos reales:
   - 'c' → Modo continuo para ver datos en tiempo real
   - 't' → Ajustar trigger para sincronizar con tu señal
   - '+'/'-' → Ajustar amplificación según rango del sensor

EJEMPLOS DE APLICACIÓN:

- Monitor de temperatura en tiempo real
- Análisis de señales de audio (micrófono)
- Visualización de aceleración/vibración
- Monitor de luminosidad ambiente
- Análisis de señales de sensores industriales
- Captura de señales digitales (protocolos de comunicación)

*/