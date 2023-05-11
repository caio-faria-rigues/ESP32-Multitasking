#include "SD.h"
#include "FS.h"
#include <SPI.h>
#include <BMP280.h>
#include <Wire.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define SCK_PIN 14
#define MISO_PIN 12
#define MOSI_PIN 13
#define CS_PIN 15

#define P0 1013.25

#define BTN_CHANGE_STATUS 13

int contador = 0;
int contador2 = 0;
int sensor_values[100] = {};
int sensor_values_size = sizeof(sensor_values) / sizeof(sensor_values[0]);

SemaphoreHandle_t xMutex;

BMP280 bmp;
uint32_t pressure = 0;
float temperature = 0;

void sensor() //pega e trata os dados do sensor ultrassonico
{
  pressure = bmp.getPressure();
  temperature = bmp.getTemperature();
}

void sd_manage(fs :: FS &fs , const char * path)
{
  contador ++;
  File file = fs.open(path, FILE_APPEND);

  if(file)
  {
    for(int i = 0; i < sensor_values_size; i++)
    {
      file.print("Distância: ");
      file.println(sensor_values[i]);
      Serial.println(sensor_values[i]);
    }
    file.close();
    Serial.println("Arquivo Atualizado");
    memset(sensor_values, 0, sizeof(sensor_values));
  }
  else
  {
    Serial.println("Falha ao acessar o arquivo.");
  }
  if(contador >= 2)
  {
    File file_read = fs.open(path);
    if(file_read)
    {
      Serial.println("------------------Leitura do arquivo: ------------------");
      while(file_read.available())
      {
        Serial.write(file_read.read());
      }
      file_read.close();
      Serial.println("------------------Arquivo fechado.------------------");
    }
  }


}

void data(void *pvParameters) //callback do int sensor() para poder criar uma task
{
  while(1)
  {
    if(xMutex != NULL)
    {
      if(xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE)
      {
        while(contador2 < 100)
        {
          sensor();
          Serial.println(contador2);
          String data_line = "Temperatura: " + String(temperature) + " | Pressão: " + String(pressure);
          Serial.println(data_line);
          sensor_values[contador2] = temperature;
          contador2 ++;
        }
      }
      Serial.println("acabou a task aqui");
      xSemaphoreGive(xMutex);
      contador2 = 0;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void record(void * pvParameters) //grava os dados retornados pelo int sensor()
{
  while(1)
  {
    Serial.println(" Chegou aqui ------------------------------------------------");
    if(xMutex != NULL)
    {
      if(xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE)
      {
        sd_manage(SD, "/teste . txt");
      }
      xSemaphoreGive(xMutex);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  xMutex = xSemaphoreCreateMutex();
  Serial.begin(115200);
  delay(1000);
  Serial.println("Inicializando o cartão SD...");
  SPIClass spi = SPIClass (HSPI);
  spi.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);

  if(!SD.begin(CS_PIN, spi, 80000000))
  {
    Serial.println("Cartão SD não encontrado.");
    return;
  }
  Serial.println("Cartão SD encontrado.");
  
  Wire.begin();
  bmp.begin();

  xTaskCreate(data, "task 1", 3000, NULL, 1, NULL);
  xTaskCreate(record, "task 2", 3000, NULL, 1, NULL);
}

void loop()
{

}
