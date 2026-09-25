#include <Arduino.h>

#define PIN_LED 10

// Prototipos de función
void Tarea1(void *parameter);
void Tarea2(void *parameter);

void setup() {
  // 1. Inicializar Hardware primero
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  delay(1000); // Dar tiempo al puerto Serie para estabilizarse

  // 2. Crear las Tareas
  xTaskCreate(
    Tarea1,      // Función de la tarea
    "TareaTexto",// Nombre descriptivo
    2048,        // Stack ajustado (bytes)
    NULL,        // Parámetros
    1,           // Prioridad
    NULL         // Handle
  );

  xTaskCreate(
    Tarea2,      // Función de la tarea
    "TareaLED",  // Nombre descriptivo
    2048,        // Stack ajustado (bytes)
    NULL,        // Parámetros
    1,           // Prioridad
    NULL         // Handle
  );
}

void loop() {
  // En RTOS, si loop() no hace nada, simplemente libera el tiempo de CPU
  vTaskDelay(pdMS_TO_TICKS(1000));
}

// DEFINICIÓN DE TAREAS

void Tarea1(void *parameter) {
  for (;;) {
    Serial.println("HOLA MUNDOOOOOO");
    vTaskDelay(pdMS_TO_TICKS(1000)); // Retardo no bloqueante
  }
}

void Tarea2(void *parameter) {
  for (;;) {
    digitalWrite(PIN_LED, LOW);
    vTaskDelay(pdMS_TO_TICKS(200));
    digitalWrite(PIN_LED, HIGH);
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}