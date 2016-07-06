//какие датчики подключены
#define temperatureIsActive true // true - если подключен,  false - если выключен
#define humidityIsActive true // true - если подключен,  false - если выключен
#define soilIsActive true // true - если подключен,  false - если выключен
#define temperatureDelay 10000 //время опроса датчика температуры в мс
#define humidityDelay 10000 //время опроса датчика влажности в мс
#define soilDelay 10000 //время опроса датчика влажности в мс

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

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <dht11.h>
#include <RTC.h>


//КОНСТАНТЫ
//==============================================================================
//названия ключей
#define keyTemperature "temperature"
#define keyHumidity "humidity"
#define keySoil "soil"

//адреса модемов (сендер отправляет все данные на main controller)
const byte mainControllerAddr[6] = "00001";

//модем для отправки данных
RF24 radio(7, 8);

//датчик температуры
dht11 DHT; // датчик температуры
#define DHT11_PIN 2 //подключен ко второму пину

//датчик влажности почвы
int soilPin = A1;

//создаем экземпляры класса для проверка температуры и модема
Checker cTemp;
Checker cHum;
Checker cSoil;

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
  if (temperatureIsActive) {
    //если прошло достаточно времени, считываем данные с датчика температуры
    if (cTemp.needToCheck(temperatureDelay)){
      sendData(keyTemperature, getTemperature());     
    }
  }

  if (humidityIsActive) {
    //если прошло достаточно времени, считываем данные с датчика температуры
    if (cHum.needToCheck(humidityDelay)){
      sendData(keyHumidity, getHumidity());
    }
  }

  if (soilIsActive) {
    //если прошло достаточно времени, считываем данные с датчика температуры
    if (cSoil.needToCheck(soilDelay)){
      sendData(keySoil, getSoil());
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

int getTemperature() {
  int chk = DHT.read(DHT11_PIN); //берем данные с датчика температуры
  return DHT.temperature;
}

int getHumidity() {
  int chk = DHT.read(DHT11_PIN);
  return DHT.humidity;
}

int getSoil() {
  return analogRead(soilPin);
}
