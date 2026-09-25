#include <Arduino.h>
#include <Wire.h>

#define DIRECCION_ESCLAVO 0x08
#define PIN_SDA 8
#define PIN_SCL 9
#define PIN_LED 10

// Estructura de datos
struct DatosAcel {
  float ax, ay, az;
};

// Cola (Queue) para pasar los datos de la tarea de muestreo a la de transmisión
QueueHandle_t colaAcelerometro;

// Prototipos de las tareas
void TareaMuestreo(void *parameter);
void TareaComunicacion(void *parameter);

void setup() {
  // 1. Inicialización de periféricos
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  // Configuración del bus I2C como Maestro
  Wire.setPins(PIN_SDA, PIN_SCL);
  Wire.begin();

  // 2. Creación de la Cola de FreeRTOS (capacidad para 10 muestras)
  colaAcelerometro = xQueueCreate(10, sizeof(DatosAcel));

  if (colaAcelerometro == NULL) {
    Serial.println("Error al crear la cola en FreeRTOS");
    while (1);
  }

  // 3. Creación de las tareas
  // Tarea de Muestreo: Pide datos cada 100 ms (Prioridad 2 - Alta)
  xTaskCreatePinnedToCore(
    TareaMuestreo,
    "MuestreoI2C",
    3072,
    NULL,
    2,
    NULL,
    1
  );

  // Tarea de Comunicación: Procesa datos cada 1 s (Prioridad 1 - Normal)
  xTaskCreatePinnedToCore(
    TareaComunicacion,
    "EnvioUART",
    4096,
    NULL,
    1,
    NULL,
    1
  );
}

void loop() {
  // El loop queda libre para que FreeRTOS gestione el tiempo de CPU
  vTaskDelay(pdMS_TO_TICKS(1000));
}

// -----------------------------------------------------------------------------
// TAREA 1: Pide datos por I2C a la Nano cada 100 ms (10 Hz)
// -----------------------------------------------------------------------------
void TareaMuestreo(void *parameter) {
  TickType_t xUltimoTiempo = xTaskGetTickCount();
  const TickType_t xFrecuencia = pdMS_TO_TICKS(100);
  DatosAcel muestraRecibida;

  for (;;) {
    // Solicitar 12 bytes a la Nano
    uint8_t bytesRecibidos = Wire.requestFrom(DIRECCION_ESCLAVO, sizeof(DatosAcel));

    if (bytesRecibidos == sizeof(DatosAcel)) {
      Wire.readBytes((uint8_t*)&muestraRecibida, sizeof(DatosAcel));
      
      // Enviar la muestra leída a la cola
      xQueueSend(colaAcelerometro, &muestraRecibida, 0);
    } else {
      // Limpieza del búfer en caso de error
      while (Wire.available()) Wire.read();
    }

    // Retardo periódico estricto a 100 ms
    vTaskDelayUntil(&xUltimoTiempo, xFrecuencia);
  }
}

// -----------------------------------------------------------------------------
// TAREA 2: Reúne 10 muestras (1 s), transmite por UART y enciende el LED 200 ms
// -----------------------------------------------------------------------------
void TareaComunicacion(void *parameter) {
  DatosAcel paquete[10];

  for (;;) {
    // Esperar hasta acumular 10 muestras de la cola
    for (int i = 0; i < 10; i++) {
      xQueueReceive(colaAcelerometro, &paquete[i], portMAX_DELAY);
    }

    // Encender LED indicativo
    digitalWrite(PIN_LED, HIGH);

    // Enviar por el puerto serie
    Serial.println("\n--- [PAQUETE 1 SEG - 10 MUESTRAS SOLICITADAS A LA NANO] ---");
    for (int i = 0; i < 10; i++) {
      Serial.print("Muestra "); Serial.print(i + 1);
      Serial.print(" | AccX: "); Serial.print(paquete[i].ax, 2);
      Serial.print(" g | AccY: "); Serial.print(paquete[i].ay, 2);
      Serial.print(" g | AccZ: "); Serial.print(paquete[i].az, 2);
      Serial.println(" g");
    }
    Serial.println("------------------------------------------------------------");

    // Mantener el LED encendido durante 200 ms sin bloquear el muestreo de I2C
    vTaskDelay(pdMS_TO_TICKS(200));
    digitalWrite(PIN_LED, LOW);
  }
}