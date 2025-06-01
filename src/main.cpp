#include <Arduino.h>
#include <NimBLEDevice.h>
#include <math.h>

// Direcciones MAC de los beacons
const char* addrAlpha = "68:5E:1C:2B:65:29";   // Beacon Alpha
const char* addrBeta = "68:5E:1C:2B:68:86";    // Beacon Beta
const char* addrCharlie = "68:5E:1C:26:E3:77"; // Beacon Charlie

// Posiciones físicas de los beacons en metros
const float BEACON_ALPHA_X = 0.0;    // Coordenada X del Beacon Alpha
const float BEACON_ALPHA_Y = 0.0;    // Coordenada Y del Beacon Alpha
const float BEACON_BETA_X = 1.0;     // Coordenada X del Beacon Beta
const float BEACON_BETA_Y = 0.0;     // Coordenada Y del Beacon Beta
const float BEACON_CHARLIE_X = 0.0;  // Coordenada X del Beacon Charlie
const float BEACON_CHARLIE_Y = 1.0;  // Coordenada Y del Beacon Charlie

// Variables globales de posición - VALORES POR DEFECTO
float posicionX = 1.0;  // Posición X actual
float posicionY = 1.0;  // Posición Y actual

// Variables para almacenar datos de RSSI
int rssiAlpha = 0;
int rssiBeta = 0;
int rssiCharlie = 0;
bool encontradoAlpha = false;
bool encontradoBeta = false;
bool encontradoCharlie = false;

// Control de tiempo para actualizaciones
unsigned long ultimaActualizacion = 0;
const unsigned long INTERVALO_ACTUALIZACION = 2000; // 2 segundos

NimBLEScan* pBLEScan;

// Parámetros de calibración individuales para cada beacon
// CALIBRAR ESTOS VALORES: Colocar cada beacon a 1 metro y ajustar
const float TX_POWER_ALPHA = -75.0;    // RSSI de referencia a 1m para Alpha
const float TX_POWER_BETA = -91.0;     // RSSI de referencia a 1m para Beta  
const float TX_POWER_CHARLIE = -75.0;  // RSSI de referencia a 1m para Charlie
const float EXPONENTE_PERDIDA = 2.0;   // Factor de pérdida de señal

/**
 * Calcula la distancia en metros basándose en el valor RSSI
 * Utiliza parámetros de calibración específicos para cada beacon
 */
float calcularDistanciaAlpha(int rssi) {
  if (rssi == 0) return -1.0;
  if (rssi >= TX_POWER_ALPHA) return 1;
  
  float ratio = (TX_POWER_ALPHA - rssi) / (10.0 * EXPONENTE_PERDIDA);
  float distancia = pow(10, ratio);
  
  if (distancia < 1.0) distancia *= 0.85;
  else if (distancia > 3.0) distancia *= 1.15;
  
  return constrain(distancia, 0.3, 20.0);
}

float calcularDistanciaBeta(int rssi) {
  if (rssi == 0) return -1.0;
  if (rssi >= TX_POWER_BETA) return 1;
  
  float ratio = (TX_POWER_BETA - rssi) / (10.0 * EXPONENTE_PERDIDA);
  float distancia = pow(10, ratio);
  
  if (distancia < 1.0) distancia *= 0.85;
  else if (distancia > 3.0) distancia *= 1.15;
  
  return constrain(distancia, 0.3, 20.0);
}

float calcularDistanciaCharlie(int rssi) {
  if (rssi == 0) return -1.0;
  if (rssi >= TX_POWER_CHARLIE) return 1;
  
  float ratio = (TX_POWER_CHARLIE - rssi) / (10.0 * EXPONENTE_PERDIDA);
  float distancia = pow(10, ratio);
  
  if (distancia < 1.0) distancia *= 0.85;
  else if (distancia > 3.0) distancia *= 1.15;
  
  return constrain(distancia, 0.3, 20.0);
}

/**
 * Calcula la posición mediante trilateración usando tres beacons
 * Resuelve el sistema de ecuaciones de círculos intersectantes
 */
bool calcularTrilateration(float distAlpha, float distBeta, float distCharlie, float& x, float& y) {
    // Verificar que todas las distancias sean válidas
    if (distAlpha <= 0 || distBeta <= 0 || distCharlie <= 0) {
        Serial.println("ERROR: Distancias inválidas para trilateración");
        return false;
    }
    
    // Obtener coordenadas de los beacons
    float x1 = BEACON_ALPHA_X, y1 = BEACON_ALPHA_Y;    // Alpha
    float x2 = BEACON_BETA_X, y2 = BEACON_BETA_Y;      // Beta
    float x3 = BEACON_CHARLIE_X, y3 = BEACON_CHARLIE_Y; // Charlie
    
    // Resolver sistema de ecuaciones mediante método algebraico
    // Basado en las ecuaciones de círculos:
    // (x-x1)² + (y-y1)² = r1²
    // (x-x2)² + (y-y2)² = r2²
    // (x-x3)² + (y-y3)² = r3²
    
    float A = 2 * (x2 - x1);
    float B = 2 * (y2 - y1);
    float C = pow(distAlpha, 2) - pow(distBeta, 2) - pow(x1, 2) + pow(x2, 2) - pow(y1, 2) + pow(y2, 2);
    
    float D = 2 * (x3 - x2);
    float E = 2 * (y3 - y2);
    float F = pow(distBeta, 2) - pow(distCharlie, 2) - pow(x2, 2) + pow(x3, 2) - pow(y2, 2) + pow(y3, 2);
    
    // Calcular determinante del sistema
    float determinante = A * E - B * D;
    
    if (abs(determinante) < 0.0001) {
        Serial.println("ERROR: Beacons colineales, imposible calcular posición única");
        return false;
    }
    
    // Resolver para x e y
    x = (C * E - F * B) / determinante;
    y = (A * F - D * C) / determinante;
    
    return true;
}

/**
 * Calcula el error promedio de la trilateración
 * Compara las distancias calculadas con las medidas
 */
float calcularErrorTrilateration(float x, float y, float distAlpha, float distBeta, float distCharlie) {
    float distCalculadaAlpha = sqrt(pow(x - BEACON_ALPHA_X, 2) + pow(y - BEACON_ALPHA_Y, 2));
    float distCalculadaBeta = sqrt(pow(x - BEACON_BETA_X, 2) + pow(y - BEACON_BETA_Y, 2));
    float distCalculadaCharlie = sqrt(pow(x - BEACON_CHARLIE_X, 2) + pow(y - BEACON_CHARLIE_Y, 2));
    
    float errorAlpha = abs(distCalculadaAlpha - distAlpha);
    float errorBeta = abs(distCalculadaBeta - distBeta);
    float errorCharlie = abs(distCalculadaCharlie - distCharlie);
    
    return (errorAlpha + errorBeta + errorCharlie) / 3.0;
}

/**
 * Callback para procesar dispositivos BLE encontrados durante el escaneo
 */
class CallbackDispositivosEncontrados: public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* dispositivo) {
      String direccionDispositivo = dispositivo->getAddress().toString().c_str();
      direccionDispositivo.toUpperCase();
      
      String direccionAlpha = String(addrAlpha);
      String direccionBeta = String(addrBeta);
      String direccionCharlie = String(addrCharlie);
      direccionAlpha.toUpperCase();
      direccionBeta.toUpperCase();
      direccionCharlie.toUpperCase();
      
      // Identificar y almacenar RSSI del beacon correspondiente
      if (direccionDispositivo == direccionAlpha) {
        rssiAlpha = dispositivo->getRSSI();
        encontradoAlpha = true;
      }
      else if (direccionDispositivo == direccionBeta) {
        rssiBeta = dispositivo->getRSSI();
        encontradoBeta = true;
      }
      else if (direccionDispositivo == direccionCharlie) {
        rssiCharlie = dispositivo->getRSSI();
        encontradoCharlie = true;
      }
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("=== SISTEMA DE LOCALIZACIÓN BLE INICIADO ===");
  Serial.println("Configuración de Beacons:");
  Serial.println("----------------------------------------");
  
  // Mostrar configuración de posiciones de beacons
  Serial.print("Beacon Alpha:   (");
  Serial.print(BEACON_ALPHA_X, 1);
  Serial.print(", ");
  Serial.print(BEACON_ALPHA_Y, 1);
  Serial.println(")");
  
  Serial.print("Beacon Beta:    (");
  Serial.print(BEACON_BETA_X, 1);
  Serial.print(", ");
  Serial.print(BEACON_BETA_Y, 1);
  Serial.println(")");
  
  Serial.print("Beacon Charlie: (");
  Serial.print(BEACON_CHARLIE_X, 1);
  Serial.print(", ");
  Serial.print(BEACON_CHARLIE_Y, 1);
  Serial.println(")");
  Serial.println();
  
  // Inicializar sistema BLE
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new CallbackDispositivosEncontrados());
  pBLEScan->setActiveScan(true);    // Escaneo activo para mejor detección
  pBLEScan->setInterval(100);       // Intervalo de escaneo
  pBLEScan->setWindow(99);          // Ventana de escaneo
  
  Serial.print("Posición inicial: (");
  Serial.print(posicionX, 1);
  Serial.print(", ");
  Serial.print(posicionY, 1);
  Serial.println(")");
  Serial.println("Esperando detección de beacons...\n");
  
  delay(1000);
}

void loop() {
  unsigned long tiempoActual = millis();
  
  // Ejecutar actualización cada INTERVALO_ACTUALIZACION milisegundos
  if (tiempoActual - ultimaActualizacion >= INTERVALO_ACTUALIZACION) {
    ultimaActualizacion = tiempoActual;
    
    // Reiniciar variables de detección
    encontradoAlpha = false;
    encontradoBeta = false;
    encontradoCharlie = false;
    rssiAlpha = 0;
    rssiBeta = 0;
    rssiCharlie = 0;
    
    // Realizar escaneo BLE por 1 segundo
    pBLEScan->start(1, false);
    
    // === MOSTRAR ESTADO DE CONEXIÓN DE BEACONS ===
    Serial.println("Estado de Conexión:");
    Serial.print("Alpha: ");
    Serial.print(encontradoAlpha ? "CONECTADO" : "DESCONECTADO");
    if (encontradoAlpha) {
      Serial.print(" | RSSI: ");
      Serial.print(rssiAlpha);
      Serial.print("dBm | Distancia: ");
      Serial.print(calcularDistanciaAlpha(rssiAlpha), 2);
      Serial.print("m");
    }
    Serial.println();
    
    Serial.print("Beta: ");
    Serial.print(encontradoBeta ? "CONECTADO" : "DESCONECTADO");
    if (encontradoBeta) {
      Serial.print(" | RSSI: ");
      Serial.print(rssiBeta);
      Serial.print("dBm | Distancia: ");
      Serial.print(calcularDistanciaBeta(rssiBeta), 2);
      Serial.print("m");
    }
    Serial.println();
    
    Serial.print("Charlie: ");
    Serial.print(encontradoCharlie ? "CONECTADO" : "DESCONECTADO");
    if (encontradoCharlie) {
      Serial.print(" | RSSI: ");
      Serial.print(rssiCharlie);
      Serial.print("dBm | Distancia: ");
      Serial.print(calcularDistanciaCharlie(rssiCharlie), 2);
      Serial.print("m");
    }
    Serial.println();
    Serial.println("----------------------------------------");
    
    // === LÓGICA PRINCIPAL DE CÁLCULO ===
    // Solo calcular nueva posición si los 3 beacons están conectados
    if (encontradoAlpha && encontradoBeta && encontradoCharlie) {
      Serial.println("✓ TODOS LOS BEACONS DETECTADOS - CALCULANDO POSICIÓN");
      
      // Calcular distancias a cada beacon usando funciones individuales
      float distanciaAlpha = calcularDistanciaAlpha(rssiAlpha);
      float distanciaBeta = calcularDistanciaBeta(rssiBeta);
      float distanciaCharlie = calcularDistanciaCharlie(rssiCharlie);
      
      Serial.print("Distancias medidas: Alpha=");
      Serial.print(distanciaAlpha, 2);
      Serial.print("m, Beta=");
      Serial.print(distanciaBeta, 2);
      Serial.print("m, Charlie=");
      Serial.print(distanciaCharlie, 2);
      Serial.println("m");
      
      // Intentar calcular nueva posición mediante trilateración
      float nuevaX, nuevaY;
      if (calcularTrilateration(distanciaAlpha, distanciaBeta, distanciaCharlie, nuevaX, nuevaY)) {
        // Calcular calidad del cálculo
        float error = calcularErrorTrilateration(nuevaX, nuevaY, distanciaAlpha, distanciaBeta, distanciaCharlie);
        
        // Actualizar variables globales con nueva posición
        posicionX = nuevaX;
        posicionY = nuevaY;
        
        Serial.print("✓ NUEVA POSICIÓN CALCULADA: (");
        Serial.print(posicionX, 2);
        Serial.print(", ");
        Serial.print(posicionY, 2);
        Serial.print(")m");
        
        // Mostrar calidad del cálculo
        if (error < 0.5) {
          Serial.println(" [EXCELENTE]");
        } else if (error < 1.0) {
          Serial.println(" [BUENA]");
        } else if (error < 2.0) {
          Serial.println(" [REGULAR]");
        } else {
          Serial.println(" [POBRE]");
        }
        
        Serial.print("Error promedio: ");
        Serial.print(error, 2);
        Serial.println("m");
        
      } else {
        Serial.println("✗ ERROR EN TRILATERACIÓN - Manteniendo posición anterior");
      }
      
    } else {
      Serial.println("⚠ BEACONS INSUFICIENTES - Manteniendo última posición conocida");
    }
    
    // === MOSTRAR POSICIÓN FINAL ACTUAL ===
    Serial.println("========================================");
    Serial.print("POSICIÓN ACTUAL: (");
    Serial.print(posicionX, 2);
    Serial.print(", ");
    Serial.print(posicionY, 2);
    Serial.println(")m");
    Serial.println("========================================\n");
    
    // Limpiar resultados del escaneo para la próxima iteración
    pBLEScan->clearResults();
  }
  
  delay(200); // Pequeña pausa para no saturar el procesador
}