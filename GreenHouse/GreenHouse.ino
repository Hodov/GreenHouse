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

//адреса модемов (пока модем получатель везде одинаковый)
const byte mainControllerAddr[6] = "00001";
const byte tempretureSwitcherAddr[6] = "00001";


//массив, где хранятся значения с датчиков, напр. values[kTemperature] это values[1], там храним температуру
int values[3] = {0,0,0};

//константы для дневной и ночной температуры
#define dayTemperature 22
#define nightTemperature 18

//константы, во сколько начинается и заканчивается день
#define startDay 8
#define endDay 22

//первый модем
RF24 radio1(7, 8);
//второй модем
RF24 radio2(9, 10);


//фоторезистор
const int pinPhoto = A0;
int raw = 0;

//инициализируем датчик времени
RTC time;

//датчик температуры
dht11 DHT; // датчик температуры
#define DHT11_PIN 2 //подключен ко второму пину

//создаем экземпляры класса для проверка температуры и модема
Checker cTemp;
Checker cModem;
Checker cPhoto;

//светодиод
int greenPin = 6;

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

  //светодиод
  pinMode(greenPin, OUTPUT);
  digitalWrite(greenPin, HIGH);
  
  // первый модем работает на отправку
  radio1.begin();
  radio1.setRetries(15, 15);
  radio1.openWritingPipe(mainControllerAddr);
  radio1.stopListening();

  // второй модем работает на прием
  radio2.begin();
  radio2.openReadingPipe(0, mainControllerAddr);
  radio2.startListening();

  //фоторезистор
  pinMode(pinPhoto, INPUT);

  //реле
  pinMode(rele, OUTPUT);
  
  //отправляем в мониторинг значения по умолчанию для реле
  sendToPortInt(nTempSwitchPos, 0);
  
}
//==========================================================
//=========================LOOP=============================
void loop()
{
  //освещенность
  if (cPhoto.needToCheck(timePeriod)) {
      getLight();
  }
  
  //если прошло больше 1 минуты, считываем данные с датчика температуры и отправляем через модем
  if (cModem.needToCheck(timePeriod)){
    sendData(nTemperature, getTemperature(), radio1, mainControllerAddr);     
    sendData(nHumidity, getHumidity(), radio1, mainControllerAddr);
    //sendToPortStr("time", getTime());
    sendToPortInt(nTemperature, getTemperature());
    sendToPortInt(nHumidity, getHumidity());
  }
  
  if (radio2.available())
  {
    char text2[32] = {0};
    radio2.read(&text2, sizeof(text2));  
    parseData(text2);
  }
  
  //  проверяем значение температуры из массива на пригодность каждые 1 мин
  if (cTemp.needToCheck(timePeriod)) {
     //внутри этой функции надо включить или выключить обогреватель, проверки уже есть
     checkTemperature(values[kTemperature], setTemperature(), radio1);
  }

  delay(1000);
  
}
//==========================================================

char sendData(String key, int par, RF24 radio, const byte *addr) {
   radio.openWritingPipe(addr);
   radio.stopListening();
   char buffer[32] = {0};
   String s = key + ";" + String(par) + ";";
   s.toCharArray(buffer,50);
   radio.write(&buffer, sizeof(buffer));
}

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
      //сохраняем в массив в зависимости от типа ключа
      if  (key == nTemperature) {
        values[kTemperature] = value;
      } else if (key == nHumidity) {
        values[kHumidity] = value;        
      } else if (key == nTempSwitchPos) {
        values[kTempSwitchPos] = value;
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


//здесь будет функция, которая будет отправлять команду на включение реле
//на одной ардуине это не проверить
void sendCommandSwitchPos(String key, RF24 radio, const byte *addr, int pos) {
  
}

// после получения команды необходимо включить/выключить реле
void turnOnOffSwitcher() {
  
}

int getTemperature() {
   int chk = DHT.read(DHT11_PIN); //берем данные с датчика температуры
   return DHT.temperature;
}

int getHumidity() {
  int chk = DHT.read(DHT11_PIN);
   return DHT.humidity;
}

int getLight() {
  return analogRead(pinPhoto);  
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

