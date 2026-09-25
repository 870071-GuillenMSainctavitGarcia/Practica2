#include <Arduino.h>
#define PIN_LED 10

void Tarea1( void * parameter )
{

    for(;;){

        Serial.println("HOLA MUNDOOOOOO");
        delay(1000);
    }
    vTaskDelete( NULL );

}

void Tarea2( void * parameter)
{
    for(;;){
      digitalWrite(PIN_LED, LOW);
      delay(200);
      digitalWrite(PIN_LED, HIGH);
      delay(200);
    }
  
    vTaskDelete( NULL );
}
void setup() {

  Serial.begin(115200);
  delay(1000);

  xTaskCreate(Tarea1,"Tarea1",4096,NULL,1,NULL);
  xTaskCreate(Tarea2,"Tarea2",10000,NULL,1,NULL);
  pinMode(PIN_LED, OUTPUT);  
}

void loop() {
  delay(1000);
}
