//разные датчики температуры и влажности не стоит использовать оба одновременно

//какие датчики подключены
#define temperatureIsActive_DHT11 false // true - если подключен,  false - если выключен
#define humidityIsActive_DHT11 false // true - если подключен,  false - если выключен
#define temperatureIsActive_DHT22 true // true - если подключен,  false - если выключен
#define humidityIsActive_DHT22 true // true - если подключен,  false - если выключен
#define soilIsActive true // true - если подключен,  false - если выключен
#define lightIsActive true // true - если подключен,  false - если выключен
#define temperatureDelay 10000 //время опроса датчика температуры в мс
#define humidityDelay 10000 //время опроса датчика влажности в мс
#define soilDelay 10000 //время опроса датчика влажности в мс
#define lightDelay 10000 //время опроса датчика освещенности в мс

class Checker
{
  // время предыдущей проверки
  unsigned long prevMillis = 0;
  
  public:
  Checker()
  {
    prevMillis = 0;
  }
  
  //как частро проверять по времени
  //раз в какое-то время millis переполняется и начинается отсчет с нуля, поэтому мы также проверяем вторым условием, если миллис начали сначала
  boolean needToCheck(long period) {   
    unsigned long currentMillis = millis();
    if ((currentMillis - prevMillis >= period)  || (currentMillis < prevMillis)) {
      prevMillis = currentMillis;
      return true;
    } else {
      return false;
    }
  }
};

#include <Wire.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <dht11.h>
#include <DHT.h>
#include <BH1750.h>


//КОНСТАНТЫ
//==============================================================================
//названия ключей
#define keyTemperature "temperature"
#define keyHumidity "humidity"
#define keySoil "soil"
#define keyLight "light"

//адреса модемов (сендер отправляет все данные на main controller)
const byte mainControllerAddr[6] = "00001";

//данные для датчика освещенности
int BH1750_address = 0x23; // i2c Addresse
int RelayIntensity  = 8;
byte buff[2];

//модем для отправки данных
RF24 radio(7, 8);

//датчик температуры
dht11 dht_11; // датчик температуры
#define DHT11_PIN 2 //подключен ко второму пину

//другой датчик DHT
#define DHTPIN 5
#define DHTTYPE DHT22
DHT dht_22(DHTPIN, DHTTYPE);

//датчик влажности почвы
int soilPin = A1;

//датчик освещенности
BH1750 lightMeter;

//создаем экземпляры класса для проверка температуры и модема
Checker cTemp;
Checker cHum;
Checker cSoil;
Checker cLight;

//=========================SETUP============================
void setup()
{
  delay(300);
  Serial.begin(9600); 
  
  // модем работает на отправку
  radio.begin();
  radio.setRetries(15, 15);
  radio.openWritingPipe(mainControllerAddr);
  radio.stopListening();
  
}
//==========================================================
//=========================LOOP=============================
void loop()
{
  // если проверяем температуру
  if (temperatureIsActive_DHT11) {
    //если прошло достаточно времени, считываем данные с датчика температуры
    if (cTemp.needToCheck(temperatureDelay)){
      sendData(keyTemperature, getTemperatureDHT11());     
    }
  }

  if (humidityIsActive_DHT11) {
    //если прошло достаточно времени, считываем данные с датчика температуры
    if (cHum.needToCheck(humidityDelay)){
      sendData(keyHumidity, getHumidityDHT11());
    }
  }

  // если проверяем температуру
  if (temperatureIsActive_DHT22) {
    //если прошло достаточно времени, считываем данные с датчика температуры
    if (cTemp.needToCheck(temperatureDelay)){
      sendData(keyTemperature, getTemperatureDHT22());     
    }
  }

  if (humidityIsActive_DHT22) {
    //если прошло достаточно времени, считываем данные с датчика температуры
    if (cHum.needToCheck(humidityDelay)){
      sendData(keyHumidity, getHumidityDHT22());
    }
  }


  if (soilIsActive) {
    //если прошло достаточно времени, считываем данные с датчика температуры
    if (cSoil.needToCheck(soilDelay)){
      sendData(keySoil, getSoil());
    }
  }

  if (lightIsActive) {
    //если прошло достаточно времени, считываем данные с датчика температуры
    if (cLight.needToCheck(lightDelay)){
      sendData(keyLight, getLight());
    }
  }
  
  
  delay(1000);
  
}
//==========================================================

char sendData(String key, int par) {
   char buffer[32] = {0};
   String s = key + ";" + String(par) + ";";
   s.toCharArray(buffer,50);
   radio.write(&buffer, sizeof(buffer));
}

// проверка температуры для датчика 11
int getTemperatureDHT11() {
  int chk = dht_11.read(DHT11_PIN);
  return dht_11.temperature;
}

// проверка влажности для датчика 11
int getHumidityDHT11() {
  int chk = dht_11.read(DHT11_PIN);
  return dht_11.humidity;
}

int getTemperatureDHT22() {  
  return dht_22.readTemperature();
}

int getHumidityDHT22() {
  return dht_22.readHumidity();
}

int getSoil() {
  return analogRead(soilPin);
}

int getLight() {
  uint16_t lux = lightMeter.readLightLevel();
  return int(lux);
}
