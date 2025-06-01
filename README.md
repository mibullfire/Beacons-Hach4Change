# 📍 Sistema de Localización BLE con Trilateración

Un sistema de posicionamiento en tiempo real basado en beacons Bluetooth Low Energy (BLE) utilizando el algoritmo de trilateración para ESP32.

## 🌟 Características

- **Localización precisa** mediante trilateración con 3 beacons BLE
- **Calibración individual** para cada beacon optimizando la precisión
- **Monitoreo en tiempo real** del estado de conexión y calidad de señal
- **Evaluación automática** de la calidad del cálculo de posición
- **Interfaz serial** con información detallada y organizada
- **Manejo robusto de errores** y validación de datos

## 🔧 Requisitos

### Hardware
- **ESP32** (cualquier variante compatible)
- **3 Beacons BLE** (mínimo para trilateración)
- Computadora con puerto serie para monitoreo

### Software
- [PlatformIO](https://platformio.org/) - Plataforma de desarrollo
- Framework Arduino para ESP32
- Biblioteca NimBLE-Arduino

## 📦 Instalación

### 1. Clonar el repositorio
```bash
git clone https://github.com/tu-usuario/ble-localization-system.git
cd ble-localization-system
```

### 2. Instalar PlatformIO
Si no tienes PlatformIO instalado:
```bash
# Opción 1: VS Code Extension
# Instala la extensión "PlatformIO IDE" en Visual Studio Code

# Opción 2: CLI
pip install platformio
```

### 3. Abrir proyecto
```bash
# Con VS Code
code .

# O con PlatformIO CLI
pio run
```

## ⚙️ Configuración

### 1. Configurar direcciones MAC de los beacons
Edita las siguientes líneas en `main.cpp`:

```cpp
const char* addrAlpha = "68:5E:1C:2B:65:29";   // Tu Beacon Alpha
const char* addrBeta = "68:5E:1C:2B:68:86";    // Tu Beacon Beta
const char* addrCharlie = "68:5E:1C:26:E3:77"; // Tu Beacon Charlie
```

### 2. Definir posiciones físicas de los beacons
Ajusta las coordenadas según tu configuración espacial:

```cpp
const float BEACON_ALPHA_X = 0.0;    // Posición X del Beacon Alpha
const float BEACON_ALPHA_Y = 0.0;    // Posición Y del Beacon Alpha
const float BEACON_BETA_X = 1.0;     // Posición X del Beacon Beta
const float BEACON_BETA_Y = 0.0;     // Posición Y del Beacon Beta
const float BEACON_CHARLIE_X = 0.0;  // Posición X del Beacon Charlie
const float BEACON_CHARLIE_Y = 1.0;  // Posición Y del Beacon Charlie
```

### 3. Calibrar potencia de transmisión (TX Power)
Para mayor precisión, coloca cada beacon a exactamente **1 metro** del ESP32 y ajusta estos valores:

```cpp
const float TX_POWER_ALPHA = -75.0;    // RSSI a 1m para Alpha
const float TX_POWER_BETA = -91.0;     // RSSI a 1m para Beta  
const float TX_POWER_CHARLIE = -75.0;  // RSSI a 1m para Charlie
```

## 🚀 Uso

### 1. Compilar y subir
```bash
# Con PlatformIO CLI
pio run --target upload

# Con VS Code: Ctrl+Alt+U (o Cmd+Alt+U en Mac)
```

### 2. Monitorear salida serie
```bash
# Con PlatformIO CLI
pio device monitor

# Con VS Code: usar el monitor serie integrado
```

### 3. Interpretar resultados
El sistema mostrará información como:

```
Estado de Conexión:
Alpha: CONECTADO | RSSI: -65dBm | Distancia: 2.34m
Beta: CONECTADO | RSSI: -72dBm | Distancia: 3.12m
Charlie: CONECTADO | RSSI: -58dBm | Distancia: 1.89m
----------------------------------------
✓ NUEVA POSICIÓN CALCULADA: (1.25, 0.87)m [EXCELENTE]
Error promedio: 0.23m
========================================
POSICIÓN ACTUAL: (1.25, 0.87)m
========================================
```

## 📊 Calidad de Cálculo

El sistema evalúa automáticamente la precisión:

- **🟢 EXCELENTE** (< 0.5m): Posicionamiento muy preciso
- **🟡 BUENA** (0.5-1.0m): Posicionamiento confiable
- **🟠 REGULAR** (1.0-2.0m): Posicionamiento aceptable
- **🔴 POBRE** (> 2.0m): Revisar configuración o posición de beacons

## 🔍 Troubleshooting

### Problema: Beacons no detectados
- Verificar que las direcciones MAC sean correctas
- Asegurar que los beacons estén encendidos y en rango
- Revisar que no haya interferencias BLE

### Problema: Posición incorrecta
- Recalibrar valores TX_POWER con beacons a 1 metro
- Verificar que las posiciones físicas estén correctamente definidas
- Asegurar que los beacons no estén colineales

### Problema: "Beacons colineales"
- Los 3 beacons deben formar un triángulo, no una línea recta
- Reposicionar al menos uno de los beacons

## 🛠️ Estructura del Proyecto

```
.
├── src/
│   └── main.cpp           # Código principal
├── include/               # Archivos de cabecera
├── lib/                   # Bibliotecas locales
├── platformio.ini         # Configuración PlatformIO
└── README.md             # Este archivo
```

## 📚 Algoritmo de Trilateración

El sistema utiliza trilateración para determinar la posición resolviendo el sistema de ecuaciones:

- **(x-x₁)² + (y-y₁)² = r₁²**
- **(x-x₂)² + (y-y₂)² = r₂²**
- **(x-x₃)² + (y-y₃)² = r₃²**

Donde:
- `(x₁,y₁), (x₂,y₂), (x₃,y₃)` son las posiciones conocidas de los beacons
- `r₁, r₂, r₃` son las distancias calculadas desde el RSSI
- `(x,y)` es la posición desconocida a calcular