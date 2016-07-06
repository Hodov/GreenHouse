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
#define kTemperature 0
#define nTemperature "temperature"
#define kHumidity 1
#define nHumidity "humidity"
#define kTempSwitchPos 2
#define nTempSwitchPos "TempSwitchPos"
#define timePeriod 300000
#define kSoil 3
#define nSoil "soil"
#define kLight 4
#define nLight "light"

//адреса модемов
const byte mainControllerAddr[6] = "00001";
const byte tempretureSwitcherAddr[6] = "00002";

//массив, где хранятся значения с датчиков, напр. values[kTemperature] это values[1], там храним температуру
int values[5] = {0,0,0,0,0};

//константы для дневной и ночной температуры
#define dayTemperature 22
#define nightTemperature 18

//константы, во сколько начинается и заканчивается день
#define startDay 8
#define endDay 22

//первый модем
RF24 radioReceiver(7, 8);
//второй модем
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
  time.begin(RTC_DS1302,5,3,4);
  //можно установить время ()
  //time.settime(0,18,10,26,05,16,4);  
  
  // первый модем работает на отправку
  radioTransmitter.begin();
  radioTransmitter.setRetries(15, 15);
  radioTransmitter.openWritingPipe(tempretureSwitcherAddr);
  radioTransmitter.stopListening();

  // первый модем работает на прием
  radioReceiver.begin();
  radioReceiver.openReadingPipe(0, mainControllerAddr);
  radioReceiver.startListening();

  //отправляем в мониторинг значения по умолчанию для реле
  sendToPortInt(nTempSwitchPos, 0);
  
}
//==========================================================
//=========================LOOP=============================
void loop()
{
  
  if (radioReceiver.available())
  {
    char msgReceive[32] = {0};
    radioReceiver.read(&msgReceive, sizeof(msgReceive));  
    parseData(msgReceive);
  }
  
  //  проверяем значение температуры из массива на пригодность каждые 1 мин
  if (cTemp.needToCheck(timePeriod)) {
     //внутри этой функции надо включить или выключить обогреватель, проверки уже есть
     checkTemperature(values[kTemperature], setTemperature(), radioTransmitter);
  }

  delay(1000);
  
}
//==========================================================

void parseData(char input[]) {
  String str; 
  str = input; //запишем, что пришло с модема
  int a = 0;  //положение последней ;
  String key; //название ключа
  int value;  //значение ключа

  //пробегаем всю строку
  for(int i = 0; i < str.length(); i++) { 
    //если нашли точку с запятой, записываем ключ
    if (str[i] == ';') {      
        key = str.substring(a,i);     
      a=i+1;
        i++;
      //ищем следующую точку с запятой и записываем значение
      while (str[i] != ';') {
        i++;
      }
      value = str.substring(a,i).toInt();     
      a=i+1;
      
      //отправляем данные в порт
      sendToPortInt(key, value);
      
      //сохраняем в массив в зависимости от типа ключа
      if  (key == nTemperature) {
        values[kTemperature] = value;
      } else if (key == nHumidity) {
        values[kHumidity] = value;        
      } else if (key == nTempSwitchPos) {
        values[kTempSwitchPos] = value;
      } else if (key == nSoil) {
        values[kSoil] = value;
      } else if (key == nLight) {
        values[kLight] = value;
      } else {        
      }
    }     
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

