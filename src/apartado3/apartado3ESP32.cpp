#include <Arduino.h>
#include <Wire.h>
#include "esp_sleep.h"
#include "esp_freertos_hooks.h" 

#define DIRECCION_ESCLAVO 0x08
#define PIN_SDA 8
#define PIN_SCL 9
#define PIN_LED 10

struct DatosAcel {
  float ax, ay, az;
};

QueueHandle_t colaAcelerometro;

void TareaMuestreo(void *parameter);
void TareaComunicacion(void *parameter);

volatile uint32_t contadorEntradasIdle = 0;

// 1. FUNCIÓN CALLBACK DEL IDLE HOOK
// Debe retornar un bool (true para indicar a FreeRTOS que continue ejecutando otros hooks)
// 1. FUNCIÓN CALLBACK DEL IDLE HOOK CORREGIDA
bool miIdleHook(void) {
  contadorEntradasIdle++;
  
  // En lugar de esp_light_sleep_start() (que congela el puerto USB Serie y los Ticks),
  // se usa la instrucción de arquitectura para pausar la CPU hasta la próxima interrupción:
  #if defined(CONFIG_IDF_TARGET_ESP32S3)
    asm volatile("waiti 0"); // Pausa el núcleo de la ESP32-S3 hasta la siguiente interrupción
  #endif

  return true; 
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  // 2. REGISTRAR EL IDLE HOOK EN ESP32
  esp_register_freertos_idle_hook(miIdleHook);

  Wire.setPins(PIN_SDA, PIN_SCL);
  Wire.begin();

  colaAcelerometro = xQueueCreate(10, sizeof(DatosAcel));

  if (colaAcelerometro == NULL) {
    Serial.println("Error al crear la cola en FreeRTOS");
    while (1);
  }

  xTaskCreatePinnedToCore(TareaMuestreo, "MuestreoI2C", 3072, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(TareaComunicacion, "EnvioUART", 4096, NULL, 1, NULL, 1);
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}

// -----------------------------------------------------------------------------
// TAREAS (se mantienen igual)
// -----------------------------------------------------------------------------
void TareaMuestreo(void *parameter) {
  TickType_t xUltimoTiempo = xTaskGetTickCount();
  const TickType_t xFrecuencia = pdMS_TO_TICKS(100);
  DatosAcel muestraRecibida;

  for (;;) {
    uint8_t bytesRecibidos = Wire.requestFrom(DIRECCION_ESCLAVO, sizeof(DatosAcel));

    if (bytesRecibidos == sizeof(DatosAcel)) {
      Wire.readBytes((uint8_t*)&muestraRecibida, sizeof(DatosAcel));
      xQueueSend(colaAcelerometro, &muestraRecibida, 0);
    } else {
      while (Wire.available()) Wire.read();
    }

    vTaskDelayUntil(&xUltimoTiempo, xFrecuencia);
  }
}

void TareaComunicacion(void *parameter) {
  DatosAcel paquete[10];

  for (;;) {
    for (int i = 0; i < 10; i++) {
      xQueueReceive(colaAcelerometro, &paquete[i], portMAX_DELAY);
    }

    digitalWrite(PIN_LED, HIGH);

    Serial.println("\n--- [PAQUETE 1 SEG - ESP32 DORMIDA EN TIEMPOS IDLE] ---");
    for (int i = 0; i < 10; i++) {
      Serial.print("Muestra "); Serial.print(i + 1);
      Serial.print(" | AccX: "); Serial.print(paquete[i].ax, 2);
      Serial.print(" g | AccY: "); Serial.print(paquete[i].ay, 2);
      Serial.print(" g | AccZ: "); Serial.print(paquete[i].az, 2);
      Serial.println(" g");
    }
    Serial.print("Veces que se durmió en el último segundo (Idle Hook): ");
    Serial.println(contadorEntradasIdle);
    contadorEntradasIdle = 0;
    Serial.println("------------------------------------------------------------");

    vTaskDelay(pdMS_TO_TICKS(200));
    digitalWrite(PIN_LED, LOW);
  }
}