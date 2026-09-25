#include <Arduino.h>
#include <Arduino_LSM9DS1.h>
#include <Wire.h>

#define DIRECCION_ESCLAVO 0x08

// Estructura de datos (12 bytes)
struct DatosAcel {
  float ax, ay, az;
};

// Variable global donde guardamos la última muestra
volatile DatosAcel ultimaLectura;

// Callback ISR: Se activa cuando la ESP32 pide datos mediante Wire.requestFrom()
void responderPeticionI2C() {
  Wire.write((uint8_t*)&ultimaLectura, sizeof(DatosAcel));
}

void setup() {
  Serial.begin(115200);

  // Inicialización de la IMU interna de la Nano 33 BLE
  if (!IMU.begin()) {
    Serial.println("¡Error al inicializar la IMU!");
    while (1);
  }

  // Configuración del bus I2C como Esclavo
  Wire.begin(DIRECCION_ESCLAVO);
  Wire.onRequest(responderPeticionI2C);
}

void loop() {
  // Muestreo continuo para tener la lectura más reciente en memoria
  if (IMU.accelerationAvailable()) {
    DatosAcel temp;
    IMU.readAcceleration(temp.ax, temp.ay, temp.az);
    
    // Copia elemento a elemento para evitar el error con la variable volatile
    noInterrupts();
    ultimaLectura.ax = temp.ax;
    ultimaLectura.ay = temp.ay;
    ultimaLectura.az = temp.az;
    interrupts();
  }
}