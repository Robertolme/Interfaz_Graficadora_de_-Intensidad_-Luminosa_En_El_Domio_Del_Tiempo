# Sistema de Osciloscopio Digital ESP32

Este proyecto implementa un sistema de osciloscopio digital basado en ESP32 que lee datos de memoria RAM vía SPI y los procesa para generar visualizaciones en tiempo real.

## Descripción del Sistema

El sistema está compuesto por varios módulos especializados:

### 🔧 **RamReader** 
Módulo encargado de leer datos de memoria RAM externa vía comunicación SPI. Compatible con memorias estándar como 25LC256, AT25256, etc.

### 📊 **PDS (Processing and Display System)**
Sistema de procesamiento de datos estilo osciloscopio que incluye:
- **Detección de Trigger**: Flancos ascendentes, descendentes, o nivel específico
- **Escalado de Amplitud**: Amplificación o atenuación de señales
- **Escalado Temporal**: Control de resolución temporal
- **Salida Matricial**: Genera matrices de 800x600 píxeles para visualización

### 🧪 **Simulador de RAM**
ESP32 adicional que simula una memoria RAM para pruebas del sistema principal.

## Estructura del Proyecto

```
MCU/
├── MCU.ino              # Programa principal del osciloscopio
├── main2.ino            # Simulador de RAM para pruebas
├── ram_reader/          # Módulo lector de RAM vía SPI
│   ├── ram_reader.h
│   └── ram_reader.cpp
├── pds/                 # Sistema de procesamiento de datos
│   ├── pds.h
│   └── pds.cpp
├── display/             # Módulo de visualización (stub)
│   ├── display.h
│   └── display.cpp
├── uart_comm/           # Comunicación UART (stub)
│   ├── uart_comm.h
│   └── uart_comm.cpp
├── peripherals/         # Periféricos adicionales (stub)
│   ├── peripherals.h
│   └── peripherals.cpp
└── net/                 # Conectividad de red (stub)
    ├── net.h
    └── net.cpp
```

## Configuración Hardware

### ESP32 Principal (Osciloscopio)
```
Conexiones SPI Master:
- MOSI: GPIO 23
- MISO: GPIO 19  
- SCK:  GPIO 18
- CS:   GPIO 5
```

### ESP32 Simulador (main2.ino)
```
Conexiones SPI Slave:
- MOSI: GPIO 23 (entrada)
- MISO: GPIO 19 (salida)
- SCK:  GPIO 18 (entrada)
- CS:   GPIO 5  (entrada)
```

## Uso del Sistema

### 1. Configuración Inicial

```cpp
// Crear instancias
RamReader ram(5);  // CS en GPIO 5
Pds pds;

// Inicializar en setup()
ram.begin();
pds.begin();

// Configurar parámetros del osciloscopio
pds.setTrigger(Pds::TRIGGER_RISING, 128);  // Trigger en flanco ascendente
pds.setAmplitudeScale(1.0f);               // Sin escalado
pds.setTimeScale(1);                       // Una muestra por píxel
```

### 2. Captura y Procesamiento

```cpp
uint8_t sampleBuffer[1024];
uint8_t displayMatrix[600][800];

// Leer datos de RAM
ram.readBlock(0x0000, sampleBuffer, 1024);

// Procesar con PDS
bool success = pds.processData(sampleBuffer, 1024, displayMatrix);

if (success) {
    // La matriz displayMatrix contiene la visualización lista para mostrar
    // displayMatrix[y][x] = intensidad del píxel (0-255)
}
```

### 3. Comandos del Sistema Principal

Una vez cargado el programa principal, usar estos comandos por Serial Monitor:

| Comando | Descripción |
|---------|-------------|
| `s` | Captura única (single shot) |
| `c` | Activar modo continuo |
| `t` | Cambiar tipo de trigger (OFF→RISING→FALLING→LEVEL→OFF) |
| `a` | Cambiar dirección de RAM (cicla entre secciones de prueba) |
| `+` | Aumentar escala de amplitud ×1.25 |
| `-` | Disminuir escala de amplitud ×0.8 |
| `h` | Mostrar ayuda |

## Funcionalidades del PDS

### Tipos de Trigger Disponibles

#### TRIGGER_OFF
- Sin sincronización
- Muestra datos de forma continua desde el inicio del buffer

#### TRIGGER_RISING  
- Detecta flancos ascendentes
- Inicia visualización cuando la señal cruza el nivel de trigger hacia arriba

#### TRIGGER_FALLING
- Detecta flancos descendentes  
- Inicia visualización cuando la señal cruza el nivel de trigger hacia abajo

#### TRIGGER_LEVEL
- Detecta cruce de nivel en cualquier dirección
- Útil para señales DC o de baja frecuencia

### Escalado de Amplitud
```cpp
pds.setAmplitudeScale(2.0f);   // Amplifica señal 2x
pds.setAmplitudeScale(0.5f);   // Atenúa señal a la mitad
pds.setAmplitudeScale(1.0f);   // Sin cambio (por defecto)
```

### Escalado Temporal
```cpp
pds.setTimeScale(1);    // Máxima resolución (1 muestra = 1 píxel)
pds.setTimeScale(4);    // Compresión 4:1 (4 muestras = 1 píxel)
pds.setTimeScale(10);   // Compresión 10:1 para señales largas
```

## Datos de Prueba del Simulador

El simulador (main2.ino) genera diferentes tipos de señales para probar el sistema:

| Dirección | Tipo de Señal | Descripción |
|-----------|---------------|-------------|
| 0x0000-0x1FFF | Onda Sinusoidal | Senoide pura de 100 muestras/ciclo |
| 0x2000-0x3FFF | Diente de Sierra | Rampa que se repite cada 256 valores |
| 0x4000-0x5FFF | Onda Cuadrada | Alterna entre 55 y 200 cada 128 muestras |
| 0x6000-0x7FFF | Ruido Aleatorio | Valores pseudo-aleatorios |
| 0x8000-0x9FFF | Rampa Lineal | Incremento lineal de 0 a 255 |
| 0xA000-0xBFFF | Datos con Trigger | Niveles específicos para probar trigger |
| 0xC000-0xDFFF | Patrón de Prueba | Secuencia conocida de 16 valores |
| 0xE000-0xFFFF | Área Reservada | Datos de reserva (0xFF) |

## Ejemplo de Uso Completo

### 1. Cargar Simulador
```bash
# Cargar main2.ino en un ESP32 (simulador de RAM)
# Conectar según el esquema de hardware
```

### 2. Cargar Sistema Principal  
```bash
# Cargar MCU.ino en otro ESP32 (osciloscopio)
# Conectar SPI entre ambos ESP32
```

### 3. Interactuar por Serial
```
=== Sistema de Osciloscopio ESP32 ===
> c                    # Activar modo continuo
> a                    # Cambiar a datos sinusoidales
> t                    # Configurar trigger ascendente
> +                    # Amplificar señal
> s                    # Captura única
```

## Personalización para Principiantes

### Agregar Nuevos Tipos de Trigger

```cpp
// En pds.h, agregar nuevo enum:
enum TriggerMode {
    TRIGGER_OFF,
    TRIGGER_RISING,
    TRIGGER_FALLING, 
    TRIGGER_LEVEL,
    TRIGGER_CUSTOM     // ← Nuevo tipo
};

// En pds.cpp, agregar lógica en findTrigger():
case TRIGGER_CUSTOM:
    // Tu código personalizado aquí
    break;
```

### Modificar Resolución de Salida

```cpp
// En pds.h, cambiar constantes:
static constexpr uint16_t MATRIX_WIDTH = 1024;   // ← Cambiar ancho
static constexpr uint16_t MATRIX_HEIGHT = 768;   // ← Cambiar alto
```

### Agregar Filtros de Señal

```cpp
// Ejemplo: filtro de media móvil simple
uint8_t filteredValue = (inputData[i-1] + inputData[i] + inputData[i+1]) / 3;
```

## Troubleshooting

### ❌ "RamReader no está listo"
- Verificar conexiones SPI
- Verificar que el simulador esté funcionando
- Comprobar pin CS (debe estar conectado)

### ❌ "Trigger no detectado"
- Ajustar nivel de trigger (comando Serial)
- Verificar que la señal cruce el nivel configurado
- Probar con TRIGGER_OFF para ver datos sin sincronización

### ❌ "Error al procesar datos"
- Verificar que hay datos válidos en el buffer
- Comprobar tamaño del buffer de entrada
- Verificar inicialización del PDS

### ❌ Matriz de salida vacía
- Verificar escalado de amplitud (no muy pequeño)
- Comprobar que las coordenadas estén en rango
- Verificar datos de entrada no sean todos iguales

## Dependencias

- **Arduino IDE** o **PlatformIO**
- **ESP32 Arduino Core** 
- **SPI Library** (incluida en Arduino Core)

## Licencia

Este proyecto es de código abierto y está disponible para uso educativo y comercial.

---
*Desarrollado para ESP32 con Arduino IDE - Sistema de Osciloscopio Digital*