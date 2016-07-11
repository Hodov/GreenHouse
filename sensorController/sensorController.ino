int controllerNumber = 1;                    //номер сенсонр контроллера, которое будет добавляться в отправляемые данные

//первый адрес свой, нужен уникальный для каждого сенсор контроллера, второй адрес главного контроллера
byte addresses[][6] = {"1sens","mainC"};

                                            //разные датчики температуры и влажности не стоит использовать одновременно
                                            //какие датчики подключены
                                            // true - если подключен,  false - если выключен
#define temperatureIsActive_DHT11 true 
#define humidityIsActive_DHT11 true         // температура в составе датчик DHT11
#define temperatureIsActive_DHT22 false     // температура в составе датчик DHT22
#define humidityIsActive_DHT22 false        // влажность в составе датчик DHT22
#define soilIsActive true                   // датчик влажности почвы
#define lightIsActive false                 // датчик света
#define photoIsActive true                  // фоторезистор
#define temperatureDelay 10000              //время опроса датчика температуры в мс
#define humidityDelay 10000                 //время опроса датчика влажности в мс
#define soilDelay 10000                     //время опроса датчика влажности в мс
#define lightDelay 10000                    //время опроса датчика освещенности в мс
#define photoDelay 10000                    //время опроса датчика освещенности в мс


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
#define keyPhoto "photo"
#define nTemperature 1
#define nHumidity 2
#define nSoil 3
#define nLight 4
#define nPhoto 5

struct Sensor {
  int controllerNumber;
  int key;
  int value; 
};

//адреса модемов (сендер отправляет все данные на main controller)
//const byte mainControllerAddr[6] = "mainC";
//byte addresses[][6] = {selfModemAddr,mainControllerAddr};

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

//фоторезистор
const int pinPhoto = A0;

//создаем экземпляры класса для проверка температуры и модема
Checker cTemp;
Checker cHum;
Checker cSoil;
Checker cLight;
Checker cPhoto;

//=========================SETUP============================
void setup()
{
  delay(300);
  Serial.begin(9600);

  // инициализация датчика света
  lightMeter.begin();

  // модем работает на отправку
  radio.begin();
  //radio.setRetries(15, 15);
  radio.openReadingPipe(1, addresses[0]);
  radio.openWritingPipe(addresses[1]);
  radio.startListening();

  //инициализация фоторезистора
  pinMode(pinPhoto, INPUT);

}
//==========================================================
//=========================LOOP=============================
void loop()
{
  // если проверяем температуру
  if (temperatureIsActive_DHT11) {
    //если прошло достаточно времени, считываем данные с датчика температуры
    if (cTemp.needToCheck(temperatureDelay)) {
      
      sendSensor(makeSensor(controllerNumber, nTemperature, getTemperatureDHT11()));
      delay(500);
    }
  }

  if (humidityIsActive_DHT11) {
    //если прошло достаточно времени, считываем данные с датчика температуры
    if (cHum.needToCheck(humidityDelay)) {
      //sendData(keyHumidity, getHumidityDHT11());
      sendSensor(makeSensor(controllerNumber, nHumidity, getHumidityDHT11()));
      delay(500);
    }
  }

  // если проверяем температуру
  if (temperatureIsActive_DHT22) {
    //если прошло достаточно времени, считываем данные с датчика температуры
    if (cTemp.needToCheck(temperatureDelay)) {
      
      sendSensor(makeSensor(controllerNumber, nTemperature, getTemperatureDHT22()));
      delay(500);
    }
  }

  if (humidityIsActive_DHT22) {
    //если прошло достаточно времени, считываем данные с датчика температуры
    if (cHum.needToCheck(humidityDelay)) {
      
      sendSensor(makeSensor(controllerNumber, nHumidity, getHumidityDHT22()));
      delay(500);
    }
  }

  if (soilIsActive) {
    //если прошло достаточно времени, считываем данные с датчика температуры
    if (cSoil.needToCheck(soilDelay)) {
      
      sendSensor(makeSensor(controllerNumber, nSoil, getSoil()));
      delay(500);
    }
  }

  if (lightIsActive) {
    //если прошло достаточно времени, считываем данные с датчика температуры
    if (cLight.needToCheck(lightDelay)) {
      
      sendSensor(makeSensor(controllerNumber, nLight, getLight()));
      delay(500);
    }
  }

  if (photoIsActive) {
    //если прошло достаточно времени, считываем данные с датчика температуры
    if (cPhoto.needToCheck(photoDelay)) {
      sendSensor(makeSensor(controllerNumber, nPhoto, getPhoto()));
      delay(500);
    }
  }

  delay(1000);

}
//==========================================================

char sendData(String key, int par) {
  char buffer[32] = {0};
  String s = key + ";" + String(par) + ";";
  s.toCharArray(buffer, 50);
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
  String s = String(lux);
  return s.toInt();
}

int getPhoto() {
  return analogRead(pinPhoto);
}

void sendSensor(Sensor outputSensor) {
  radio.stopListening();
  if (!radio.write(&outputSensor, sizeof(outputSensor))) {
    Serial.println(F("failed"));
  }
  radio.startListening();
  sendSensorToPort(outputSensor);
}

Sensor makeSensor(int contrNumber, int key, int value) {
  Sensor newData;
  newData.controllerNumber = contrNumber;
  newData.key = key;
  newData.value = value;
  
  return newData;
}

void sendSensorToPort(Sensor data) {  
  String s = String(data.controllerNumber) + ":" + String(data.key) + ":" + String(data.value);
  Serial.println(s);
}

