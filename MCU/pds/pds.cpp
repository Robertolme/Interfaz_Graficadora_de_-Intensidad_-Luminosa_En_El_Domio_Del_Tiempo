#include "pds.h"
#include <climits>  // Para SIZE_MAX
#include <algorithm> // Para min/max

// Definir SIZE_MAX si no está disponible
#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

// Función helper para constrain si no está disponible en Arduino
template<typename T>
T constrainValue(T value, T min_val, T max_val) {
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

/**
 * @brief Constructor de la clase PDS
 * 
 * Inicializa los parámetros por defecto para el procesamiento de datos.
 */
Pds::Pds() 
    : _triggerMode(TRIGGER_OFF)
    , _triggerLevel(128)           // Nivel medio por defecto
    , _amplitudeScale(1.0f)        // Sin escalado por defecto
    , _samplesPerPixel(1)          // Una muestra por píxel
    , _initialized(false)
    , _triggerDetected(false)
    , _triggerPosition(0) {
}

/**
 * @brief Inicializa el sistema PDS
 * 
 * Prepara el sistema para el procesamiento de datos. Debe llamarse
 * antes de usar cualquier otra función.
 */
void Pds::begin() {
    _initialized = true;
    Serial.println("PDS: Sistema inicializado");
}

/**
 * @brief Configura los parámetros del trigger
 * 
 * El trigger determina en qué punto de la señal comenzar la captura
 * para obtener una visualización estable.
 * 
 * @param mode Modo de trigger:
 *        - TRIGGER_OFF: Sin trigger, muestra datos continuamente
 *        - TRIGGER_RISING: Detecta flancos ascendentes
 *        - TRIGGER_FALLING: Detecta flancos descendentes  
 *        - TRIGGER_LEVEL: Detecta cuando la señal cruza un nivel específico
 * @param level Nivel de trigger (0-255, donde 128 es el valor medio)
 */
void Pds::setTrigger(TriggerMode mode, uint8_t level) {
    _triggerMode = mode;
    _triggerLevel = level;
    Serial.printf("PDS: Trigger configurado - Modo: %d, Nivel: %d\n", mode, level);
}

/**
 * @brief Configura la escala de amplitud
 * 
 * Permite amplificar o atenuar la señal visualizada.
 * 
 * @param scale Factor de escala:
 *        - 1.0: Sin escalado (tamaño original)
 *        - >1.0: Amplifica la señal (ej: 2.0 = doble tamaño)
 *        - <1.0: Atenúa la señal (ej: 0.5 = mitad del tamaño)
 */
void Pds::setAmplitudeScale(float scale) {
    if (scale > 0.0f) {
        _amplitudeScale = scale;
        Serial.printf("PDS: Escala de amplitud: %.2f\n", scale);
    }
}

/**
 * @brief Configura la escala de tiempo
 * 
 * Determina cuántas muestras de datos se representan en cada píxel
 * horizontal de la pantalla.
 * 
 * @param samplesPerPixel Número de muestras por píxel:
 *        - 1: Máxima resolución temporal (cada muestra = 1 píxel)
 *        - >1: Compresión temporal (múltiples muestras por píxel)
 */
void Pds::setTimeScale(uint16_t samplesPerPixel) {
    if (samplesPerPixel > 0) {
        _samplesPerPixel = samplesPerPixel;
        Serial.printf("PDS: Escala de tiempo: %d muestras/píxel\n", samplesPerPixel);
    }
}

/**
 * @brief Procesa los datos de entrada y genera la matriz de salida
 * 
 * Esta es la función principal que toma los datos crudos de la memoria
 * y los convierte en una representación visual de 800x600 píxeles.
 * 
 * @param inputData Array de datos de entrada (valores de 0-255)
 * @param inputLength Número de muestras en inputData
 * @param outputMatrix Matriz de salida 800x600 donde se escribirá el resultado
 * @return true si el procesamiento fue exitoso, false en caso de error
 */
bool Pds::processData(const uint8_t* inputData, size_t inputLength, uint8_t outputMatrix[MATRIX_HEIGHT][MATRIX_WIDTH]) {
    // Verificación de parámetros de entrada
    if (!_initialized || inputData == nullptr || inputLength == 0 || outputMatrix == nullptr) {
        Serial.println("PDS: Error - Parámetros inválidos");
        return false;
    }

    // Limpiar la matriz de salida
    clearMatrix(outputMatrix);

    // Buscar trigger si está habilitado
    size_t startIndex = 0;
    _triggerDetected = false;
    
    if (_triggerMode != TRIGGER_OFF) {
        size_t triggerPos = findTrigger(inputData, inputLength);
        if (triggerPos != SIZE_MAX) {
            startIndex = triggerPos;
            _triggerDetected = true;
            _triggerPosition = triggerPos;
            Serial.printf("PDS: Trigger detectado en posición %zu\n", triggerPos);
        } else {
            Serial.println("PDS: Trigger no detectado, usando inicio de datos");
        }
    }

    // Calcular cuántos píxeles horizontales podemos llenar
    size_t availableSamples = inputLength - startIndex;
    size_t pixelsToFill = (availableSamples / _samplesPerPixel < MATRIX_WIDTH) ? 
                          availableSamples / _samplesPerPixel : MATRIX_WIDTH;

    if (pixelsToFill == 0) {
        Serial.println("PDS: Warning - No hay suficientes datos para llenar píxeles");
        return false;
    }

    // Procesar datos y dibujar la forma de onda
    uint16_t prevY = scaleAmplitude(inputData[startIndex]);
    
    for (size_t x = 1; x < pixelsToFill; x++) {
        // Calcular el índice de la muestra para este píxel
        size_t sampleIndex = startIndex + (x * _samplesPerPixel);
        
        if (sampleIndex >= inputLength) break;
        
        // Si hay múltiples muestras por píxel, tomar el promedio
        uint32_t sum = 0;
        size_t sampleCount = 0;
        
        for (size_t i = 0; i < _samplesPerPixel && (sampleIndex + i) < inputLength; i++) {
            sum += inputData[sampleIndex + i];
            sampleCount++;
        }
        
        uint8_t avgValue = (sampleCount > 0) ? (sum / sampleCount) : inputData[sampleIndex];
        uint16_t currentY = scaleAmplitude(avgValue);

        // Dibujar línea desde el punto anterior al punto actual
        drawLine(outputMatrix, x-1, prevY, x, currentY, 255);
        
        prevY = currentY;
    }

    Serial.printf("PDS: Datos procesados - %zu píxeles dibujados\n", pixelsToFill);
    return true;
}

/**
 * @brief Obtiene el estado del trigger
 * @return true si se detectó trigger en los últimos datos procesados
 */
bool Pds::getTriggerStatus() const {
    return _triggerDetected;
}

/**
 * @brief Obtiene la posición del trigger
 * @return Índice en los datos donde se detectó el trigger
 */
size_t Pds::getTriggerPosition() const {
    return _triggerPosition;
}

/**
 * @brief Busca la posición del trigger en los datos
 * 
 * Implementa diferentes algoritmos de detección según el modo configurado.
 * 
 * @param data Array de datos de entrada
 * @param length Longitud del array
 * @return Índice donde se detectó el trigger, o SIZE_MAX si no se encontró
 */
size_t Pds::findTrigger(const uint8_t* data, size_t length) {
    if (length < 2) return SIZE_MAX;

    switch (_triggerMode) {
        case TRIGGER_RISING:
            // Buscar flanco ascendente: valor anterior < nivel Y valor actual >= nivel
            for (size_t i = 1; i < length; i++) {
                if (data[i-1] < _triggerLevel && data[i] >= _triggerLevel) {
                    return i;
                }
            }
            break;

        case TRIGGER_FALLING:
            // Buscar flanco descendente: valor anterior >= nivel Y valor actual < nivel
            for (size_t i = 1; i < length; i++) {
                if (data[i-1] >= _triggerLevel && data[i] < _triggerLevel) {
                    return i;
                }
            }
            break;

        case TRIGGER_LEVEL:
            // Buscar primer cruce del nivel (cualquier dirección)
            for (size_t i = 0; i < length; i++) {
                if ((data[i] >= _triggerLevel && (i == 0 || data[i-1] < _triggerLevel)) ||
                    (data[i] < _triggerLevel && (i == 0 || data[i-1] >= _triggerLevel))) {
                    return i;
                }
            }
            break;

        case TRIGGER_OFF:
        default:
            return SIZE_MAX;
    }

    return SIZE_MAX; // Trigger no encontrado
}

/**
 * @brief Convierte valor de entrada a coordenada Y en la matriz
 * 
 * Aplica el escalado de amplitud y convierte el rango 0-255 de entrada
 * al rango 0-(MATRIX_HEIGHT-1) de salida.
 * 
 * @param value Valor de entrada (0-255)
 * @return Coordenada Y en la matriz (0 a MATRIX_HEIGHT-1)
 */
uint16_t Pds::scaleAmplitude(uint8_t value) {
    // Aplicar escalado de amplitud
    float scaledValue = value * _amplitudeScale;
    
    // Normalizar a rango 0-255
    scaledValue = constrainValue(scaledValue, 0.0f, 255.0f);
    
    // Convertir a coordenada Y (invertir porque Y=0 está arriba)
    uint16_t y = MATRIX_HEIGHT - 1 - (uint16_t)((scaledValue / 255.0f) * (MATRIX_HEIGHT - 1));
    
    return constrainValue(y, (uint16_t)0, (uint16_t)(MATRIX_HEIGHT - 1));
}

/**
 * @brief Limpia la matriz de salida
 * 
 * Inicializa todos los píxeles a 0 (fondo negro).
 * 
 * @param matrix Matriz a limpiar
 */
void Pds::clearMatrix(uint8_t matrix[MATRIX_HEIGHT][MATRIX_WIDTH]) {
    for (uint16_t y = 0; y < MATRIX_HEIGHT; y++) {
        for (uint16_t x = 0; x < MATRIX_WIDTH; x++) {
            matrix[y][x] = 0; // Fondo negro
        }
    }
}

/**
 * @brief Dibuja una línea en la matriz usando el algoritmo de Bresenham
 * 
 * Conecta dos puntos con una línea suave para crear una representación
 * continua de la forma de onda.
 * 
 * @param matrix Matriz donde dibujar
 * @param x1 Coordenada X inicial
 * @param y1 Coordenada Y inicial
 * @param x2 Coordenada X final  
 * @param y2 Coordenada Y final
 * @param value Valor a escribir en los píxeles (0-255)
 */
void Pds::drawLine(uint8_t matrix[MATRIX_HEIGHT][MATRIX_WIDTH], 
                   uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t value) {
    
    // Verificar límites
    if (x1 >= MATRIX_WIDTH || x2 >= MATRIX_WIDTH || y1 >= MATRIX_HEIGHT || y2 >= MATRIX_HEIGHT) {
        return;
    }

    // Algoritmo de Bresenham para dibujar líneas
    int16_t dx = abs((int16_t)x2 - (int16_t)x1);
    int16_t dy = abs((int16_t)y2 - (int16_t)y1);
    int16_t sx = (x1 < x2) ? 1 : -1;
    int16_t sy = (y1 < y2) ? 1 : -1;
    int16_t err = dx - dy;
    
    int16_t x = x1, y = y1;
    
    while (true) {
        // Dibujar píxel actual
        if (x >= 0 && x < MATRIX_WIDTH && y >= 0 && y < MATRIX_HEIGHT) {
            matrix[y][x] = value;
        }
        
        // Verificar si llegamos al punto final
        if (x == x2 && y == y2) break;
        
        // Calcular siguiente píxel
        int16_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}
