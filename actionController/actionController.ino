//адрес мейн контроллера и свой
byte addresses[][6] = {"mainC", "10000"};


//НАГРЕВАТЕЛЬ
#define heaterRele 10
//ОХЛАДИТЕЛЬ
#define coolingRele 11
//УВЛАЖНИТЕЛЬ
#define humidifierRele 12
//ОСВЕТИТЕЛЬ
#define lightningRele 13

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

//описание названия и расположения в массиве для хранения данных с датчиков
#define turnOn 1
#define turnOff 0
#define kHeaterRelePosition 6
#define nHeaterRelePosition "HeaterRelePosition"
#define kHumidifierRelePosition 7
#define nHumidifierRelePosition "HumidifierRelePosition"
#define kWaterinRelePosition 8
#define nWaterinRelePosition "WateringRelePosition"
#define kCoolingRelePosition 9
#define nCoolingRelePosition "CoolingRelePosition"
#define kSoilRelePosition 9
#define nSoilRelePosition "SoilRelePosition"
#define kLightRelePosition 10
#define nLightRelePosition "LightRelePosition"

struct Sensor {
  int controllerNumber = 0;
  int key;
  int value;
};

//модем
RF24 radio(7, 8);

Sensor input;
//=========================SETUP============================
void setup()
{
  delay(300);
  Serial.begin(9600);

  // модем работает на прием
  radio.begin();
  radio.openReadingPipe(0, addresses[1]);
  radio.startListening();

  //перечисление реле
  pinMode(heaterRele, OUTPUT);
  pinMode(coolingRele, OUTPUT);
  pinMode(humidifierRele, OUTPUT);
  pinMode(lightningRele, OUTPUT);

}
//==========================================================
//=========================LOOP=============================
void loop()
{

  if (radio.available())
  {
    radio.read(&input, sizeof(input));
    sendSensorToPort(input);
    makeAction(input);
  }

  delay(100);

}
//==========================================================

// после получения команды необходимо включить/выключить реле
void makeAction(Sensor sens) {
  int releNum = 0;
  if (sens.key == kHeaterRelePosition) {
    releNum = heaterRele;
  } else if (sens.key == kHumidifierRelePosition) {
    releNum = humidifierRele;
  } else if (sens.key == kCoolingRelePosition) {
    releNum = coolingRele;
  } else if (sens.key == kLightRelePosition) {
    releNum = lightningRele;
  }
  if (sens.value == turnOn) {
    digitalWrite(releNum, HIGH);
  } else if (sens.value == turnOff) {
    digitalWrite(releNum, LOW);
  }
}

void sendSensorToPort(Sensor data) {
  String keyStr;
  if (data.key == kHeaterRelePosition) {
    keyStr = nHeaterRelePosition;
  } else if (data.key == kHumidifierRelePosition) {
    keyStr = nHumidifierRelePosition;
  } else if (data.key == kWaterinRelePosition) {
    keyStr = nWaterinRelePosition;
  } else if (data.key == kCoolingRelePosition) {
    keyStr = nCoolingRelePosition;
  } else if (data.key == kSoilRelePosition) {
    keyStr = nSoilRelePosition;
  } else if (data.key == kLightRelePosition) {
    keyStr = nLightRelePosition;
  }
  String output = String(data.controllerNumber) + ";" + keyStr + ";" + String(data.value) + ";";
  Serial.println(output);
}

