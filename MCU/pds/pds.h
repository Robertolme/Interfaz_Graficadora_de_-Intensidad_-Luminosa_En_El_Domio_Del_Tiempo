#ifndef PDS_H
#define PDS_H

#include <Arduino.h>

/**
 * @brief PDS (Processing and Display System) - Sistema de procesamiento de datos estilo osciloscopio
 * 
 * Esta clase procesa datos capturados de memoria RAM y los convierte en una matriz
 * de 800x600 píxeles para su visualización, similar a un osciloscopio.
 * 
 * Funcionalidades principales:
 * - Detección de trigger por nivel y pendiente
 * - Escalado de amplitud configurable
 * - Escalado de tiempo configurable
 * - Generación de matriz de salida 800x600
 */
class Pds {
public:
    // Constantes para la matriz de salida
    static constexpr uint16_t MATRIX_WIDTH = 800;   // Ancho de la matriz de salida
    static constexpr uint16_t MATRIX_HEIGHT = 600;  // Alto de la matriz de salida
    
    // Tipos de trigger
    enum TriggerMode {
        TRIGGER_OFF,        // Sin trigger
        TRIGGER_RISING,     // Flanco ascendente
        TRIGGER_FALLING,    // Flanco descendente
        TRIGGER_LEVEL       // Nivel específico
    };

    /**
     * @brief Constructor de la clase PDS
     */
    Pds();

    /**
     * @brief Inicializa el sistema PDS
     */
    void begin();

    /**
     * @brief Configura los parámetros del trigger
     * @param mode Modo de trigger (TRIGGER_OFF, TRIGGER_RISING, TRIGGER_FALLING, TRIGGER_LEVEL)
     * @param level Nivel de trigger (0-255)
     */
    void setTrigger(TriggerMode mode, uint8_t level);

    /**
     * @brief Configura la escala de amplitud
     * @param scale Factor de escala (1.0 = sin escalado, >1.0 amplifica, <1.0 atenúa)
     */
    void setAmplitudeScale(float scale);

    /**
     * @brief Configura la escala de tiempo
     * @param samplesPerPixel Cuántas muestras representar en cada píxel horizontal
     */
    void setTimeScale(uint16_t samplesPerPixel);

    /**
     * @brief Procesa los datos de entrada y genera la matriz de salida
     * @param inputData Array de datos de entrada (valores de 0-255)
     * @param inputLength Número de muestras en inputData
     * @param outputMatrix Matriz de salida 800x600 (se debe reservar memoria externamente)
     * @return true si el procesamiento fue exitoso, false en caso de error
     */
    bool processData(const uint8_t* inputData, size_t inputLength, uint8_t outputMatrix[MATRIX_HEIGHT][MATRIX_WIDTH]);

    /**
     * @brief Obtiene información del estado actual del trigger
     * @return true si se detectó trigger en los últimos datos procesados
     */
    bool getTriggerStatus() const;

    /**
     * @brief Obtiene la posición donde se detectó el trigger
     * @return Índice en los datos de entrada donde se detectó el trigger
     */
    size_t getTriggerPosition() const;

private:
    // Parámetros de configuración
    TriggerMode _triggerMode;           // Modo de trigger actual
    uint8_t _triggerLevel;              // Nivel de trigger (0-255)
    float _amplitudeScale;              // Escala de amplitud
    uint16_t _samplesPerPixel;          // Muestras por píxel en eje X

    // Estado interno
    bool _initialized;                   // Estado de inicialización
    bool _triggerDetected;              // Estado del trigger
    size_t _triggerPosition;            // Posición del trigger

    /**
     * @brief Busca la posición del trigger en los datos
     * @param data Array de datos de entrada
     * @param length Longitud del array
     * @return Índice donde se detectó el trigger, o SIZE_MAX si no se encontró
     */
    size_t findTrigger(const uint8_t* data, size_t length);

    /**
     * @brief Convierte valor de entrada (0-255) a coordenada Y en la matriz
     * @param value Valor de entrada (0-255)
     * @return Coordenada Y en la matriz (0 a MATRIX_HEIGHT-1)
     */
    uint16_t scaleAmplitude(uint8_t value);

    /**
     * @brief Limpia la matriz de salida (la llena de ceros)
     * @param matrix Matriz a limpiar
     */
    void clearMatrix(uint8_t matrix[MATRIX_HEIGHT][MATRIX_WIDTH]);

    /**
     * @brief Dibuja una línea en la matriz desde (x1,y1) hasta (x2,y2)
     * @param matrix Matriz donde dibujar
     * @param x1 Coordenada X inicial
     * @param y1 Coordenada Y inicial  
     * @param x2 Coordenada X final
     * @param y2 Coordenada Y final
     * @param value Valor a escribir en los píxeles de la línea
     */
    void drawLine(uint8_t matrix[MATRIX_HEIGHT][MATRIX_WIDTH], 
                  uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t value);
};

#endif // PDS_H
