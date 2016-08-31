int64_t ownAddr = 0xE8E8F0F0E1LL;

int turnOnRelay = HIGH;
int turnOffRelay = LOW;

//НАГРЕВАТЕЛЬ
int heaterRele = 3;
//ОХЛАДИТЕЛЬ
int coolingRele = 6;
//УВЛАЖНИТЕЛЬ
int humidifierRele = 4;
//ОСВЕТИТЕЛЬ
int lightningRele = 5;

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
  Serial.begin(9600);

  // модем работает на прием
  radio.begin();
  radio.openReadingPipe(0, ownAddr);
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(0x76);
  radio.enableDynamicPayloads();
  radio.startListening();
  radio.powerUp();

  //перечисление реле
  pinMode(heaterRele, OUTPUT);
  pinMode(coolingRele, OUTPUT);
  pinMode(humidifierRele, OUTPUT);
  pinMode(lightningRele, OUTPUT);

  digitalWrite(heaterRele, turnOnRelay);
  digitalWrite(coolingRele, turnOnRelay);
  digitalWrite(humidifierRele, turnOnRelay);
  digitalWrite(lightningRele, turnOnRelay);

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
    Serial.print(releNum);
    Serial.println(" ON");
    digitalWrite(releNum, turnOnRelay);
  } else if (sens.value == turnOff) {
    Serial.print(releNum);
    Serial.println(" OFF");
    digitalWrite(releNum, turnOffRelay);
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

