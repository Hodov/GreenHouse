//адрес мейн контроллера
byte addresses[][6] = {"mainC"};

//какой модуль часов использовать
#define rtc_old true
#define rtc_new false

//место для сенсоров в памяти
#define maxSensors 15

//сколько раз считаем повторения значения
#define numRepetitionTemp 5
#define numRepetitionSoil 1

//как часто проверяем значения
#define temperatureCheckPeriod 60000
#define humidityCheckPeriod 60000
#define wateringCheckPeriod 60000

//константы для дневной и ночной температуры для нагрева
#define dayTemperature 22
#define nightTemperature 18
//допустимый диапазон нормального значения для сенсора
#define temperatureDelta 2

//константы для дневной и ночной температуры для охлаждения
#define dayTemperatureCooling 24
#define nightTemperatureCooling 20

//константы, во сколько начинается и заканчивается день для смены температуры
#define startDay 8
#define endDay 22

//константы для влажности
#define constHumidity 75
#define humidityDelta 5

//константы для влажности почвы
#define constSoil 400
#define soilDelta 100


struct Sensor {
  int controllerNumber = 0;
  int key;
  int value;
};

struct indicators {
  int controllerNumber = 0;
  int key;
  int value;
  int oldValue;
  int lowValueCounter[2] = {0, 0};
  int highValueCounter[2] = {0, 0};
};


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
#define kTemperature 1
#define kHumidity 2
#define kSoil 3
#define kLight 4
#define kPhoto 5

#define nTemperature "temperature"
#define nHumidity "humidity"
#define nSoil "soil"
#define nLight "light"
#define nPhoto "photo"

#define kTempSwitchPos 6
#define nHeaterRelePosition "HeaterRelePosition"
#define kTempSwitchPos 7
#define nHumidifierRelePosition "HumidifierRelePosition"
#define kWateringSwitchPos 8
#define nWaterinRelePosition "WateringRelePosition"
#define kCoolingSwitchPos 9
#define nCoolingRelePosition "CoolingRelePosition"
#define kSoilSwitchPos 9
#define nSoilRelePosition "SoilRelePosition"

//массив класса сенсоров, где будем хранить полученные значения
indicators sensorValues[maxSensors];

//первый модем
RF24 radio(7, 8);

//инициализируем датчик времени
RTC time;

//создаем экземпляры класса для проверка температуры и модема
Checker cTemp;
Checker cHumi;
Checker cWatering;

//реле
int rele = 12;

Sensor inputSensor;


unsigned long prevMillis = 0;
//=========================SETUP============================
void setup()
{
  delay(300);
  Serial.begin(9600);

  //time модуль
  if (rtc_old) {
    time.begin(RTC_DS1302, 5, 3, 4);
  } else if (rtc_new) {
    time.begin(RTC_DS3231);
  } else {
  }

  //не удалять, можно установить время
  //0 сек, 18 мин, 10 часов, 26 мая 16 года, 4 день недели
  //time.settime(0,27,10,13,07,16,3);

  // первый модем работает на прием
  radio.begin();
  radio.openReadingPipe(1, addresses[0]);
  radio.startListening();

  //отправляем в мониторинг значения по умолчанию для реле
  //sendToPortInt(nTempSwitchPos, 0);

}
//==========================================================
//=========================LOOP=============================
void loop()
{
  if (radio.available())
  {
    radio.read(&inputSensor, sizeof(inputSensor));
    sendSensorToPort(inputSensor);
    saveDataFromSensor(inputSensor);
  }

  //  проверяем значение температуры из массива на пригодность каждые 1 мин
  if (cTemp.needToCheck(temperatureCheckPeriod)) {
    checkHeater();
    checkCooling();
  }

  if (cHumi.needToCheck(humidityCheckPeriod)) {
    checkHumidifier();
  }

  if (cWatering.needToCheck(wateringCheckPeriod)) {
    //если понедельник, если время 9 часов утра
    checkWatering();
  }
  delay(1000);

}
//==========================================================

void saveDataFromSensor(Sensor data) {

  boolean matchSensor = false;

  int i = 0;
  while ((sensorValues[i].controllerNumber != 0) and (!(sensorValues[i].controllerNumber == data.controllerNumber and sensorValues[i].key == data.key)) ) {
    i++;
  }
  sensorValues[i].controllerNumber = data.controllerNumber;
  sensorValues[i].key = data.key;
  sensorValues[i].oldValue = sensorValues[i].value;
  sensorValues[i].value = data.value;
}

//проверка температурных значений для включения обогревателя
void checkSensorValue(int i, int minValue, int maxValue, int numCounter) {
  if (sensorValues[i].value > maxValue ) {
    if (sensorValues[i].oldValue > maxValue ) {
      sensorValues[i].highValueCounter[numCounter]++;
      sensorValues[i].lowValueCounter[numCounter] = 0;
    } else {
      sensorValues[i].highValueCounter[numCounter] = 1;
      sensorValues[i].lowValueCounter[numCounter] = 0;
    }
  } else if (sensorValues[i].value < minValue ) {
    if (sensorValues[i].oldValue < minValue ) {
      sensorValues[i].highValueCounter[numCounter] = 0;
      sensorValues[i].lowValueCounter[numCounter]++;
    } else {
      sensorValues[i].highValueCounter[numCounter] = 0;
      sensorValues[i].lowValueCounter[numCounter] = 1;
    }
  } else {
    sensorValues[i].highValueCounter[numCounter] = 0;
    sensorValues[i].lowValueCounter[numCounter] = 0;
  }
}

int sendAction(int i, String releName, int minValueAction, int maxValueAction, int numCounter, int repetitions) {
  if (sensorValues[i].highValueCounter[numCounter] > repetitions) {
    sendRelePosition(releName, sensorValues[i].controllerNumber, maxValueAction);
  } else if (sensorValues[i].lowValueCounter[numCounter] > repetitions) {
    sendRelePosition(releName, sensorValues[i].controllerNumber, minValueAction);
  } else {
  }
}

void checkAction(int sensorType, String releName, int minValue, int maxValue, int minValueAction, int maxValueAction, int numCounter) {
  int i = 0;
  while (sensorValues[i].controllerNumber != 0) {
    if (sensorValues[i].key == sensorType) {
      checkSensorValue(i, minValue, maxValue, numCounter);
      if ((sensorType == kTemperature) or (sensorType == kHumidity)) {
        sendAction(i, releName, minValueAction, maxValueAction, numCounter, getNumRepetitions(sensorType));
      }
    }
    i++;
  }
}

void checkTemperature(String releName, int minValue, int maxValue, int minValueAction, int maxValueAction, int numCounter) {
  checkAction(kTemperature, releName, minValue, maxValue, minValueAction, maxValueAction, numCounter);
}

void checkHeater() {
  checkTemperature(nHeaterRelePosition, getTreshold(kTemperature, "heater") - getDelta(kTemperature), getTreshold(kTemperature, "heater") + getDelta(kTemperature), turnOn, turnOff, 0);
}

void checkCooling() {
  checkTemperature(nCoolingRelePosition, getTreshold(kTemperature, "cooler") - getDelta(kTemperature), getTreshold(kTemperature, "cooler") + getDelta(kTemperature), turnOff, turnOn, 1);
}

void checkHumidity(String releName, int minValue, int maxValue, int minValueAction, int maxValueAction, int numCounter) {
  checkAction(kHumidity, releName, minValue, maxValue, minValueAction, maxValueAction, numCounter);
}

void checkHumidifier() {
  checkHumidity(nHumidifierRelePosition, getTreshold(kHumidity, "main") - getDelta(kHumidity), getTreshold(kHumidity, "main") + getDelta(kHumidity), turnOn, turnOff, 0);
}

void checkSoil() {
  checkAction(kSoil, nSoilRelePosition, getTreshold(kSoil, "main") - getDelta(kSoil), getTreshold(kSoil, "main") + getDelta(kSoil), turnOn, turnOff, 0);
}

void checkWatering() {
  if (reqTime()) {
    if (reqSoil()) {
      startWatering();

    }
  }
}

bool reqTime() {
  return true;
}

bool reqSoil() {
  return true;
}

void startWatering() {

}

int setTemperature(String purpose) {
  int hour = atoi(time.gettime("H"));
  if (hour > startDay and hour < endDay) {
    if (purpose == "heater") {
      return dayTemperature;
    } else if (purpose == "cooler") {
      return dayTemperatureCooling;
    }
  } else {
    if (purpose == "heater") {
      return nightTemperature;
    } else if (purpose == "cooler") {
      return nightTemperatureCooling;
    }
  }
}

int setHumidity() {
  return constHumidity;
}

int setSoil() {
  return constSoil;
}

void sendRelePosition(String key, int addr, int pos) {
  //здесь нужно перевести инт в байты, чтобы передать адрес отправления !!!!!!!!!!!!!!!!!!!!!!!!
  char buffer[32] = {0};
  String s = key + ";" + String(pos) + ";";
  s.toCharArray(buffer, 50);
  radio.openWritingPipe(addr);
  radio.stopListening();
  radio.write(&buffer, sizeof(buffer));
  radio.startListening();
  sendToPortInt(addr, key, pos);
}

void sendToPortInt(int addr, String key, int value) {
  String s = String(addr) + ";" + key + ";" + String(value) + ";";
  Serial.println(s);
}

void sendToPortStr(String key, String value) {
  String s = key + ";" + value + ";";
  Serial.println(s);
}

String getTime() {
  String s = String(time.gettime("H")) + ':' + String(time.gettime("i")) + ':' + String(time.gettime("s"));
  return s;
}

void sendSensorToPort(Sensor data) {
  String keyStr;
  if (data.key == kTemperature) {
    keyStr = nTemperature;
  } else if (data.key == kHumidity) {
    keyStr = nHumidity;
  } else if (data.key == kSoil) {
    keyStr = nSoil;
  } else if (data.key == kLight) {
    keyStr = nLight;
  } else if (data.key == kPhoto) {
    keyStr = nPhoto;
  } else {

  }
  String output = String(data.controllerNumber) + ";" + keyStr + ";" + String(data.value) + ";";
  Serial.println(output);
}

int getDelta(int sensorKey) {
  if (sensorKey == kTemperature) {
    return temperatureDelta;
  } else if (sensorKey == kHumidity) {
    return humidityDelta;
  } else if (sensorKey == kSoil) {
    return soilDelta;
  }
}

int getTreshold(int sensorKey, String purpose) {
  if (sensorKey == kTemperature) {
    return setTemperature(purpose);
  } else if (sensorKey == kHumidity) {
    return setHumidity();
  } else if (sensorKey == kSoil) {
    return setSoil();
  }
}

int getNumRepetitions(int sensor) {
  if (sensor == kTemperature) {
    return numRepetitionTemp;
  } else if (sensor == kHumidity) {
    return numRepetitionTemp;
  } else if (sensor == kSoil) {
    return numRepetitionSoil;
  }
}

