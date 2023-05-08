#include <SD.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define CS_PIN 5
#define DI_PIN 23
#define DO_PIN 19
#define SCK_PIN 18

#define ECHO 33
#define TRIGGER 25
#define MAX_DISTANCE 500

#define BTN_CHANGE_STATUS 13

int contador = 0;
int contador2 = 0;
int sensor_values[100] = {};
int sensor_values_size = sizeof(sensor_values) / sizeof(sensor_values[0]);

void setup(){
  Serial.begin(115200);
  Serial.println("Inicializando o cartão SD...");

  if(!SD.begin(CS_PIN)){
    Serial.println("Cartão SD não encontrado.");
    return;
  }
  Serial.println("Cartão SD encontrado.");

  xTaskCreate(data, "task 1", 1000, NULL, 1, NULL);
  xTaskCreate(record, "task 2", 3000, NULL, 1, NULL);


}

void loop(){

}

int sensor(){ //pega e trata os dados do sensor ultrassonico
  pinMode(TRIGGER, OUTPUT);
  pinMode(ECHO, INPUT);

  digitalWrite(TRIGGER, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER, LOW);

  int duration = pulseIn(ECHO, HIGH);
  int distance = duration / 58;
  
  return distance;
}

void data(void *pvParameters){ //callback do int sensor() para poder criar uma task
  while(1){
    int distance = sensor();
    Serial.println("Distância em CM: ");
    Serial.println(distance);
    sensor_values[contador2] = distance;
    contador2 ++;

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void record(void * pvParameters){ //grava os dados retornados pelo int sensor()
  while(1){
    contador ++;
    int distance = sensor();
    File file = SD.open("/teste.txt", FILE_WRITE);

    if (file){
      for(int i = 0; i < sensor_values_size; i++){
        if(sensor_values[i]!=0){
          file.print("Distância: ");
          file.println(sensor_values[i]);
        }
      }
      file.println("Distância: ");
      file.print(distance);
      file.close();
    }
    else{
      Serial.println("Falha ao acessar o arquivo.");
    }
    if (contador >= 5){
      File file_read = SD.open("/teste.txt");
      if (file_read){
        Serial.println("Do arquivo: ");
        while(file_read.available()){
          Serial.write(file_read.read());
        }
        file_read.close();
      }
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
