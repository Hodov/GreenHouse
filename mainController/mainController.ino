//адрес мейн контроллера
byte addresses[][6] = {"mainC"};

#define rtc_old true
#define rtc_new false
#define maxSensors 15

struct Sensor {
  int controllerNumber = 0;
  int key;
  int value; 
};

class Checker
{
  //сколько раз проверять, что данные действительно перешли пороговый уровень
  #define maxCounterForSwitch 5
  
  int currentState = 0;
 
  // время предыдущей проверки
  unsigned long prevMillis = 0;
 
  int currentCounter = 0;
  int switchPosition = 0;
  
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

//проверяем несколько раз показатели, чтобы убедиться, что пороговое значение перешли
boolean needToSwitch(int futureSwitchPosition, int currentPos) {
  //switchPosition = currentPos;
  if (switchPosition != futureSwitchPosition) {
    currentCounter++;
  } else {
      currentCounter = 0;
  }  
  if ((currentCounter >= maxCounterForSwitch) || (switchPosition != currentPos)){
    currentCounter = 0;
    switchPosition = futureSwitchPosition;
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
#define kLight 5

#define nTemperature "temperature"
#define nHumidity "humidity"
#define nSoil "soil"
#define nLight "light"
#define nLight "photo"

#define kTempSwitchPos 5
#define nTempSwitchPos "TempSwitchPos"
#define timePeriod 300000

//адрес модема получателя будет совпадать с названием сенсора отправителя
const byte tempretureSwitcherAddr[6] = "00002";

//удалить после исправления функции
int values[]={0};
//массив класса сенсоров, где будем хранить полученные значения
Sensor sensorsValues[maxSensors];

//константы для дневной и ночной температуры
#define dayTemperature 22
#define nightTemperature 18

//константы, во сколько начинается и заканчивается день
#define startDay 8
#define endDay 22

//первый модем
//второй модем
RF24 radioReceiver(7, 8);
RF24 radioTransmitter(9, 10);

//инициализируем датчик времени
RTC time;

//создаем экземпляры класса для проверка температуры и модема
Checker cModem;
Checker cTemp;

//реле
int rele = 12;

//=========================SETUP============================
void setup()
{
  delay(300);
  Serial.begin(9600);
  
  //time модуль
  if (rtc_old) {
    time.begin(RTC_DS1302,5,3,4);  
  } else if (rtc_new) {
    time.begin(RTC_DS3231);
  } else {
  }
  
  //можно установить время ()
  //0 сек, 18 мин, 10 часов, 26 мая 16 года, 4 день недели
  //time.settime(0,18,10,26,05,16,4);
    
  
  // первый модем работает на отправку
  radioTransmitter.begin();
  radioTransmitter.setRetries(15, 15);
  radioTransmitter.openWritingPipe(tempretureSwitcherAddr);
  radioTransmitter.stopListening();

  // первый модем работает на прием
  radioReceiver.begin();
  radioReceiver.openReadingPipe(1, addresses[0]);
  radioReceiver.startListening();

  //отправляем в мониторинг значения по умолчанию для реле
  sendToPortInt(nTempSwitchPos, 0);
  
}
//==========================================================
//=========================LOOP=============================
void loop()
{
  Sensor inputSensor;
  if (radioReceiver.available())
  {
    radioReceiver.read(&inputSensor, sizeof(inputSensor));
    sendSensorToPort(inputSensor);
    saveDataFromSensor(inputSensor);
  }
  
  //  проверяем значение температуры из массива на пригодность каждые 1 мин
  if (cTemp.needToCheck(timePeriod)) {
     //внутри этой функции надо включить или выключить обогреватель, проверки уже есть
     //checkTemperature(values[kTemperature], setTemperature(), radioTransmitter);
  }
  Serial.println("Begin");
  for (int i=0; i<maxSensors; i++) {
    
    Serial.println(sensorsValues[i].key);
    Serial.println(sensorsValues[i].value);
    
  }

  delay(1000);
  
}
//==========================================================

void saveDataFromSensor(Sensor data) {
  
  boolean matchSensor = false;

  int num = 0;
  int i = 0;
  while (sensorsValues[i].controllerNumber != 0) {

    if (sensorsValues[i].controllerNumber == data.controllerNumber and sensorsValues[i].key == data.key) {
      matchSensor = true;
      sensorsValues[i].value = data.value;
    }

    if (sensorsValues[i].controllerNumber != 0) {
      num++;
    }
    i++;
  }
  
  if (not matchSensor) {
    sensorsValues[num] = data;
  }
}

void checkTemperature(int value, int treshold, RF24 radio) { 
  // положение обогревателя при запуске программы необходимо ставить выкл
  if (value < treshold) {
    if (cTemp.needToSwitch(turnOn, values[kTempSwitchPos])) {
      // отправляем команду включить обогреватель
      sendSwitchPos(nTempSwitchPos,radio,tempretureSwitcherAddr, 1);      
    }
  } else {
    if (cTemp.needToSwitch(turnOff, values[kTempSwitchPos])) {  
    // отправляем команду включить обогреватель
      sendSwitchPos(nTempSwitchPos,radio,tempretureSwitcherAddr, 0);
    }
  }
}

int setTemperature() {
  int hour = atoi(time.gettime("H"));
  if (hour > startDay and hour < endDay) {
    return dayTemperature;
  } else {
    return nightTemperature;
  }
}

//эту функцию будет отправлять контроллер реле после включения или отключения реле
void sendSwitchPos(String key, RF24 radio, const byte *addr, int pos) {
  radio.openWritingPipe(addr);
  radio.stopListening();
  char buffer[32] = {0};
  String s = key + ";" + String(pos) + ";";
  sendToPortInt(key, pos);
  s.toCharArray(buffer,50);
  radio.write(&buffer, sizeof(buffer));
}

void sendToPortInt(String key, int value) {
  String s = key+";"+String(value)+";";
  Serial.println(s);
}

void sendToPortStr(String key, String value) {
  String s = key+";"+value+";";
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
  } else {
    
  }
  String output = String(data.controllerNumber)+";"+keyStr+";"+String(data.value)+";";
  Serial.println(output);
}

