#include <Arduino.h>

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
#define releTemperature 12
#define nTempSwitchPos "TempSwitchPos"
#define timePeriod 300000

//адреса модемов (пока модем получатель везде одинаковый)
const byte mainControllerAddr[6] = "00001";
const byte tempretureSwitcherAddr[6] = "00001";

//модем
RF24 radio(7, 8);

//реле
int rele = 12;

//=========================SETUP============================
void setup()
{
  delay(300);
  Serial.begin(9600); 
  
  // первый модем работает на отправку
  //radio1.begin();
  //radio1.setRetries(15, 15);
  //radio1.openWritingPipe(mainControllerAddr);
  //radio1.stopListening();

  // модем работает на прием
  radio.begin();
  radio.openReadingPipe(0, tempretureSwitcherAddr);
  radio.startListening();

  //перечисление реле
  pinMode(releTemperature, OUTPUT);
  
}
//==========================================================
//=========================LOOP=============================
void loop()
{
  
  if (radio.available())
  {
    char text2[32] = {0};
    radio2.read(&text2, sizeof(text2));  
    parseData(text2);
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
    }     
  }
  turnOnOffSwitcher(key, value);
}

//эту функцию будет отправлять контроллер реле после включения или отключения реле
void sendSwitchPos(String key, int value) {
  sendingRadio;
  char buffer[32] = {0};
  String s = key + ";" + String(value) + ";";
  sendToPortInt(key, value);
  s.toCharArray(buffer,50);
  radio.write(&buffer, sizeof(buffer));
  listenRadio;
}

// после получения команды необходимо включить/выключить реле
void turnOnOffSwitcher(String key, int value) {
  if (value == turnOn) {
    if (key == nTempSwitchPos) {
      digitalWrite(releTemperature, HIGH);
      sendSwitchPos(key,value);
    }
  } else if (value == turnOff) {
    if (key == nTempSwitchPos) {
      digitalWrite(releTemperature, LOW);       
      sendSwitchPos(key,value);
    } 
  }
}

void listenRadio() {
  radio.begin();
  radio.openReadingPipe(0, tempretureSwitcherAddr);
  radio.startListening();
}

void sendingRadio() {
  radio1.setRetries(15, 15);
  radio1.openWritingPipe(mainControllerAddr);
  radio1.stopListening();
}

void sendToPortInt(String key, int value) {
  String s = key+";"+String(value)+";";
  Serial.println(s);
}

void sendToPortStr(String key, String value) {
  String s = key+";"+value+";";
  Serial.println(s);
}

