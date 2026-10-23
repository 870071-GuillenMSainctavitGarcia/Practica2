# Práctica 2: Sistemas Multitarea con FreeRTOS y Gestión Energética (ESP32-S3 y Arduino Nano 33 BLE)

## Descripción General

Este proyecto implementa un sistema embebido distribuido enfocado en la concurrencia multitarea, el paso de mensajes interprocesos mediante colas, la comunicación I2C entre plataformas de desarrollo heterogéneas y la optimización del consumo energético en tiempos de inactividad utilizando FreeRTOS.

Las plataformas de desarrollo empleadas son el ESP32-S3 (como procesador maestro) y el Arduino Nano 33 BLE (como esclavo con sensor inercial integrado).

**¿Como probar los subapartados del proyecto?**

Como cada apartado es un código distinto:

Si vas a probar los Apartados 1, 2, 3 o 5, abre el archivo de la carpeta correspondiente o ajusta el filtro build_src_filter en el archivo platformio.ini para apuntar al subapartado deseado.

Si vas a probar el sistema completo I2C (Apartados 2 y 3):

Carga el archivo del nodo esclavo (apartado2Nano.cpp o apartado3Nano.cpp) en la Arduino Nano 33 BLE.

Carga el archivo del nodo maestro (apartado2ESP32.cpp o apartado3ESP32.cpp) en la ESP32-S3.

Abre el Monitor Serie a 115200 baudios.

## Estructura del Proyecto

```text
Practica2/
├── include/
│   └── Config.h               # Definiciones globales y estructuras de datos
├── src/
│   ├── apartado1/             # Multitarea básica con FreeRTOS (Tareas de Texto y LED)
│   ├── apartado2/             # Sistema I2C Maestro-Esclavo con Queues de FreeRTOS
│   │   ├── apartado2ESP32.cpp # Maestro I2C, recepción periódica y gestión de cola
│   │   └── apartado2Nano.cpp  # Esclavo I2C, muestreo continuo de IMU (LSM9DS1)
│   ├── apartado3/             # Optimización de consumo mediante Idle Hook en FreeRTOS
│   │   ├── apartado3ESP32.cpp # Maestro I2C con Idle Hook (waiti 0) y contador
│   │   └── apartado3Nano.cpp  # Esclavo I2C (mismo comportamiento de captación IMU)
│   └── apartado5/             # Comunicación inalámbrica por chat (SPP / BLE)
│       └── apartado5.cpp      # Servidor de chat serie por Bluetooth
├── platformio.ini             # Entornos de compilación y monitorización de PlatformIO
└── README.md                  # Documentación principal
```

## APARTADO 1

Procedimiento: Demostración del planificador de tareas multitarea de FreeRTOS en ESP32, alternando la ejecución de dos hilos independientes sin uso de retrasos bloqueantes.

* **Lógica y Funciones Clave:**

    * xTaskCreate(): Instancia dos tareas independientes (TareaTexto y TareaLED) asignándoles nombre, tamaño de pila (stack) de 2048 bytes y prioridad 1.
    * vTaskDelay(pdMS_TO_TICKS(1000)): Pausa la ejecución de la tarea durante un intervalo en milisegundos convirtiéndolo a ticks del sistema operativo, liberando el tiempo de CPU para otras tareas.
    * digitalWrite(PIN_LED, LOW/HIGH): Conmuta el nivel lógico del LED de forma cíclica desde la tarea dedicada a la señalización visual.
    * vTaskDelay() en loop(): Mantiene el hilo principal de Arduino libre para permitir la gestión pura de tareas por parte del kernel de FreeRTOS.

## APARTADO 2 - Maestro (ESP32-S3)

Procedimiento: Arquitectura de recepción de datos por I2C desacoplada mediante una cola (Queue) de FreeRTOS para almacenar muestras de aceleración y transmitirlas en bloque por UART.

* **Lógica y Funciones Clave:**

    * xQueueCreate(10, sizeof(DatosAcel)): Reserva memoria para una cola FIFO con capacidad para almacenar hasta 10 estructuras de datos inerciales de 12 bytes.
    * xTaskCreatePinnedToCore(): Instancia la TareaMuestreo (Prioridad 2 - Alta) y la TareaComunicacion (Prioridad 1 - Normal) asignadas explícitamente al núcleo 1 del procesador.
    * vTaskDelayUntil(): Garantiza que la TareaMuestreo se ejecute a una frecuencia estricta y determinista de 10 Hz (100 ms).
    * Wire.requestFrom() / Wire.readBytes(): Solicita y lee 12 bytes desde la dirección I2C del esclavo (0x08).

xQueueSend() / xQueueReceive(): Pasa los datos entre la tarea de muestreo y la de comunicación. La recepción utiliza portMAX_DELAY para permanecer bloqueada hasta acumular el paquete completo de 10 muestras (1 s).

## APARTADO 2 - Esclavo (Arduino Nano 33 BLE)

Procedimiento: Captura continua de los 3 ejes del acelerómetro desde la IMU interna (LSM9DS1) y respuesta inmediata a peticiones I2C mediante interrupciones por hardware.

* **Lógica y Funciones Clave:**

    * IMU.begin(): Inicializa el sensor inercial integrado sobre el bus I2C/SPI interno de la placa.
    * Wire.begin(DIRECCION_ESCLAVO): Configura el controlador I2C en modo esclavo respondiendo a la dirección física 0x08.
    * Wire.onRequest(responderPeticionI2C): Registra la rutina ISR que empaqueta y transmite la estructura ultimaLectura cuando el maestro solicita datos.
    * noInterrupts() / interrupts(): Delimita una sección crítica durante la copia de datos entre la lectura de la IMU y la variable global volatile para evitar condiciones de carrera.

## APARTADO 3 - Maestro (ESP32-S3)

Procedimiento: Implementación de un mecanismo de bajo consumo acoplado al hilo de inactividad (Idle Task) de FreeRTOS para poner la CPU en reposo en los intervalos libres de procesamiento.

* **Lógica y Funciones Clave:**

    * esp_register_freertos_idle_hook(miIdleHook): Registra una función callback que el kernel ejecuta automáticamente cada vez que no existen tareas en estado listo (Ready).
    * asm volatile("waiti 0"): Instrucción en ensamblador específica de la arquitectura Xtensa (ESP32-S3) que suspende el ciclo de la CPU hasta la llegada de la siguiente interrupción, manteniendo operativos los periféricos y temporizadores.
    * contadorEntradasIdle: Variable global que contabiliza el número de ciclos de inactividad ejecutados por segundo para medir cuantitativamente el tiempo de reposo del procesador.

## APARTADO 3 - Esclavo (Arduino Nano 33 BLE)

Procedimiento: Muestreo y provisión de datos inerciales mediante I2C en modo esclavo, manteniendo compatibilidad directa con las peticiones del maestro optimizado.

* **Lógica y Funciones Clave:**

    * IMU.accelerationAvailable() / IMU.readAcceleration(): Comprueba la disponibilidad de datos en el sensor y extrae las lecturas físicas de aceleración en los ejes X, Y y Z.
    * volatile DatosAcel ultimaLectura: Mantiene actualizada la última muestra en memoria para ser leída de forma atómica dentro del callback de interrupción.
    * Wire.write(): Transmite los 12 bytes de la estructura directamente al bus I2C tras recibir la petición del maestro.

