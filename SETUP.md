# Guía de Instalación Rápida - Sistema de Osciloscopio ESP32

Esta guía te ayudará a configurar y probar el sistema paso a paso, incluso si eres principiante.

## 📋 Requisitos

### Hardware
- 2x ESP32 (cualquier modelo compatible con Arduino)
- Cables jumper macho-macho (mínimo 4)
- Breadboard (opcional, para conexiones más organizadas)
- Cable USB para programar los ESP32

### Software
- [Arduino IDE](https://www.arduino.cc/en/software) (versión 1.8.19 o superior)
- Driver ESP32 para Arduino IDE

## 🔧 Configuración del Arduino IDE

### 1. Instalar ESP32 en Arduino IDE
```
1. Abrir Arduino IDE
2. Ir a: Archivo → Preferencias
3. En "URLs Adicionales de Gestor de Tarjetas" agregar:
   https://dl.espressif.com/dl/package_esp32_index.json
4. Ir a: Herramientas → Placa → Gestor de tarjetas
5. Buscar "esp32" e instalar "ESP32 by Espressif Systems"
```

### 2. Seleccionar la Placa ESP32
```
Herramientas → Placa → ESP32 Arduino → "ESP32 Dev Module"
```

## 🔌 Conexiones Hardware

### Opción A: Con 2 ESP32 (Prueba Completa)

**ESP32 #1 (Simulador de RAM)**
- Cargará el programa `main2.ino`

**ESP32 #2 (Osciloscopio)**  
- Cargará el programa `MCU.ino`

**Conexiones SPI entre ESP32:**
```
ESP32 Simulador  ←→  ESP32 Osciloscopio
GPIO 19 (MISO)   ←→  GPIO 19 (MISO)
GPIO 23 (MOSI)   ←→  GPIO 23 (MOSI)
GPIO 18 (SCK)    ←→  GPIO 18 (SCK)
GPIO 5  (CS)     ←→  GPIO 5  (CS)
GND              ←→  GND
```

### Opción B: Con 1 ESP32 (Solo Pruebas de Software)
- Cargar el programa `test_pds.ino`
- No requiere conexiones adicionales

## 🚀 Instalación Paso a Paso

### Para Principiantes - Prueba Simple (1 ESP32)

#### Paso 1: Descargar el Proyecto
```
1. Descargar el código desde GitHub
2. Extraer en una carpeta (ej: C:\ESP32_Oscilloscope)
```

#### Paso 2: Probar PDS sin Hardware
```
1. Abrir Arduino IDE
2. Abrir el archivo: MCU/test_pds.ino
3. Conectar ESP32 via USB
4. Seleccionar puerto en: Herramientas → Puerto
5. Hacer clic en "Subir" (flecha derecha)
6. Abrir Monitor Serie (Ctrl+Shift+M)
7. Configurar velocidad: 115200 baudios
```

**Resultado esperado:**
```
=== Prueba del Sistema PDS ===
Iniciando pruebas...
Generando datos de prueba...
Generados 500 bytes de datos de prueba

=== Prueba: Sin Trigger ===
✓ Procesamiento exitoso
✓ Píxeles dibujados: 1234

=== Prueba: Trigger Ascendente ===
✓ Procesamiento exitoso  
✓ Trigger detectado en posición 67
✓ Píxeles dibujados: 1156
```

### Para Usuarios Avanzados - Sistema Completo (2 ESP32)

#### Paso 1: Programar Simulador de RAM
```
1. Conectar primer ESP32
2. Abrir MCU/main2.ino en Arduino IDE
3. Subir programa
4. Verificar en Monitor Serie que dice "SPI Slave configurado"
```

#### Paso 2: Programar Osciloscopio
```
1. Conectar segundo ESP32
2. Abrir MCU/MCU.ino en Arduino IDE
3. Subir programa
4. Verificar en Monitor Serie la inicialización
```

#### Paso 3: Conectar Hardware
```
Seguir el diagrama de conexiones SPI mostrado arriba
```

#### Paso 4: Probar Sistema
```
1. Monitor Serie del osciloscopio a 115200 baudios
2. Debería mostrar: "Sistema de Osciloscopio ESP32"
3. Probar comandos:
   - 'c' → Modo continuo
   - 'a' → Cambiar sección de datos
   - 't' → Cambiar trigger
   - 'h' → Ayuda
```

## 🧪 Comandos de Prueba

### Comandos Básicos
```
c  → Activar modo continuo
s  → Captura única
a  → Cambiar tipo de datos (seno, sierra, cuadrada, etc.)
t  → Cambiar trigger (OFF→RISING→FALLING→LEVEL)
+  → Amplificar señal
-  → Reducir señal
h  → Mostrar ayuda
```

### Secuencia de Prueba Recomendada
```
1. Escribir 'h' → Ver ayuda
2. Escribir 'c' → Activar modo continuo
3. Escribir 'a' → Cambiar a datos sinusoidales
4. Escribir 't' → Configurar trigger ascendente
5. Escribir '+' → Amplificar para ver mejor
```

## ❌ Solución de Problemas

### Error: "RamReader no está listo"
**Causas posibles:**
- Conexiones SPI incorrectas
- Simulador no está funcionando
- Pin CS no conectado

**Solución:**
1. Verificar todas las conexiones SPI
2. Revisar que el simulador esté encendido
3. Verificar Monitor Serie del simulador

### Error: Compilación fallida
**Causas posibles:**
- ESP32 no instalado en Arduino IDE
- Puerto no seleccionado
- Placa incorrecta seleccionada

**Solución:**
1. Reinstalar ESP32 en Gestor de Tarjetas
2. Seleccionar puerto correcto
3. Verificar placa: "ESP32 Dev Module"

### Error: No se ven datos
**Causas posibles:**
- Baudios incorrectos en Monitor Serie
- ESP32 no conectado
- Programa no cargado correctamente

**Solución:**
1. Configurar Monitor Serie a 115200 baudios
2. Verificar conexión USB
3. Volver a cargar el programa

## 📊 Interpretación de Resultados

### Salida Normal del Test (test_pds.ino)
```
✓ Procesamiento exitoso       → PDS funcionando
✓ Trigger detectado          → Sistema de trigger OK  
✓ Píxeles dibujados: 1234    → Visualización generada
```

### Salida Normal del Sistema (MCU.ino)
```
Capturando 1024 bytes desde dirección 0x000000...
Primeros 16 bytes: 80 9C B7 CE E1 F0 FB FF FF FB F0 E1 CE B7 9C 80
PDS: Datos procesados exitosamente
PDS: Trigger detectado en posición 45
Matriz generada: 1456 píxeles dibujados (valor: 1-255)
```

## 🎯 Próximos Pasos

Una vez que tengas el sistema funcionando:

1. **Experimenta con diferentes señales**
   - Usar comando 'a' para cambiar entre seno, sierra, cuadrada
   
2. **Prueba diferentes triggers**
   - Usar comando 't' para cambiar modos de trigger
   
3. **Ajusta la visualización**
   - Usar '+' y '-' para cambiar amplificación
   
4. **Conecta señales reales**
   - Reemplazar simulador con memoria SPI real
   - Conectar sensores analógicos al simulador

## 📞 Soporte

Si tienes problemas:
1. Verificar que ambos ESP32 estén alimentados
2. Revisar conexiones SPI paso a paso
3. Probar primero con test_pds.ino 
4. Verificar Monitor Serie en ambos ESP32

---
*¡Felicidades! Ahora tienes un osciloscopio digital funcionando con ESP32* 🎉