#include "SD.h"
#include "FS.h"
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <Wire.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define SCK_PIN 18
#define MISO_PIN 19
#define MOSI_PIN 23
#define CS_PIN 5

#define P0 1013.25

#define BTN_CHANGE_STATUS 15

int contador = 0;
int contador2 = 0;
int cont = 0; 
int pressure_values[100] = {};
int pressure_values_size = sizeof(pressure_values) / sizeof(pressure_values[0]);
int temperature_values[100] = {};
int temperature_values_size = sizeof(temperature_values) / sizeof(temperature_values[0]);

SemaphoreHandle_t xMutex;

Adafruit_BMP280 bmp;
#define BMP_SDA 21//Definição dos pinos I2C
#define BMP_SCL 22
uint32_t pressure = 0;
float temperature = 0;

void sensor() //pega e trata os dados do sensor ultrassonico
{
  pressure = bmp.readPressure();
  temperature = bmp.readTemperature();
}

void sd_manage(fs :: FS &fs , const char * path)
{
  contador ++;
  File file = fs.open(path, FILE_APPEND);

  if(file)
  {
    for(int i = 0; i < pressure_values_size; i++)
    {
      file.print("Temperatura ");
      
      file.println(temperature_values[i]);
      file.print(" | Pressão: ");
      file.println(pressure_values[i]);
      
      Serial.print("Temperatura ");
      Serial.println(temperature_values[i]);
      Serial.print(" | Pressão: ");
      Serial.println(pressure_values[i]);
    }
    file.close();
    Serial.println("Arquivo Atualizado");
    memset(pressure_values, 0, sizeof(pressure_values));
    memset(temperature_values, 0, sizeof(temperature_values));
  }
  else
  {
    Serial.println("Falha ao acessar o arquivo.");
  }
  //if(cont % 3 == 0)
  //{
  //  File file_read = fs.open(path);
  //  if(file_read)
  //  {
  //    Serial.println("------------------Leitura do arquivo: ------------------");
  //    while(file_read.available())
  //    {
  //      Serial.write(file_read.read());
  //    }
  //    file_read.close();
  //    Serial.println("------------------Arquivo fechado.------------------");
  //  }
  //}
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
          butao();
          Serial.println(contador2);
          String data_line = "Temperatura: " + String(temperature) + " | Pressão: " + String(pressure / 1013.25);
          Serial.println(data_line);
          temperature_values[contador2] = temperature;
          pressure_values[contador2] = pressure / P0;
          contador2 ++;
        }
      }
      Serial.println("acabou a task aqui");
      xSemaphoreGive(xMutex);
      contador2 = 0;
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void record(void * pvParameters) //grava os dados retornados pelo int sensor()
{
  while(1)
  {
    Serial.println(" Chegou aqui ------------------------------------------------");
    butao();
    if(xMutex != NULL)
    {
      if(cont % 2 ==0)
      {
        if(xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE)
        {
          sd_manage(SD, "/teste . txt");
        }
      }
      else xSemaphoreGive(xMutex);
      xSemaphoreGive(xMutex);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);   
  }
}

void butao()
{
  if(!digitalRead(BTN_CHANGE_STATUS)){                                  // Verifica se o botão 1 foi pressionado
    cont++;                                              // Caso tenha incrementa +1 a cont
    while(!digitalRead(BTN_CHANGE_STATUS)){delay(10);}                // Loop até que o botão 1 seja solto
    Serial.println("Contador: ");                       // Imprime a quantidade de cont na Serial
    Serial.println(cont);
  }
}

void setup()
{
  xMutex = xSemaphoreCreateMutex();
  Serial.begin(115200);
  pinMode(BTN_CHANGE_STATUS, INPUT_PULLUP);
  delay(1000);
  Serial.println("Inicializando o cartão SD...");
  SPIClass spi = SPIClass (CS_PIN);
  spi.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);

  if(!SD.begin(CS_PIN))
  {
    Serial.println("Cartão SD não encontrado.");
    return;
  }
  Serial.println("Cartão SD encontrado.");
  
  Wire.begin();
  bmp.begin(0x76);

  xTaskCreate(data, "task 1", 3000, NULL, 1, NULL);
  xTaskCreate(record, "task 2", 3000, NULL, 1, NULL);
}

void loop()
{

}
