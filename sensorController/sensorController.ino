int controllerNumber = 10000;                    //номер сенсонр контроллера, которое будет добавляться в отправляемые данные

//первый адрес свой, нужен уникальный для каждого сенсор контроллера, второй адрес главного контроллера
byte addresses[][6] = {"1sens","mainC"};

                                            //разные датчики температуры и влажности не стоит использовать одновременно
                                            //какие датчики подключены
                                            // true - если подключен,  false - если выключен
#define temperatureIsActive_DHT11 true 
#define humidityIsActive_DHT11 true         // температура в составе датчик DHT11
#define temperatureIsActive_DHT22 false     // температура в составе датчик DHT22
#define humidityIsActive_DHT22 false        // влажность в составе датчик DHT22
#define soilIsActive false                   // датчик влажности почвы
#define lightIsActive true                 // датчик света
#define photoIsActive false                  // фоторезистор
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
RF24 radio(9, 10);

//датчик температуры
dht11 dht_11; // датчик температуры
#define DHT11_PIN 2 //подключен ко второму пину

//другой датчик DHT
#define DHTPIN 5
#define DHTTYPE DHT22
DHT dht_22(DHTPIN, DHTTYPE);

//датчик влажности почвы
int soilPin = A1;

//фоторезистор
const int pinPhoto = A0;

//датчик освещенности
BH1750 lightMeter;

//создаем экземпляры класса для проверка температуры и модема
Checker cTemp;
Checker cHum;
Checker cSoil;
Checker cLight;
Checker cPhoto;

//=========================================================================
//SLEEP MODE

#include <avr/sleep.h>
#include <avr/power.h>
#define LED_PIN (4)
volatile int f_timer = 0;

//Description: Timer1 Overflow interrupt.
ISR(TIMER1_OVF_vect)
{
  /* set the flag. */
  if (f_timer == 0)
  {
    f_timer = 1;
  }
}

//Description: Enters the arduino into sleep mode.
void enterSleep(void)
{
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();

  /* Disable all of the unused peripherals. This will reduce power
     consumption further and, more importantly, some of these
     peripherals may generate interrupts that will wake our Arduino from
     sleep!
  */
  power_adc_disable();
  power_spi_disable();
  power_timer0_disable();
  power_timer2_disable();
  power_twi_disable();
  /* Now enter sleep mode. */
  sleep_mode();
  /* The program will continue from here after the timer timeout*/
  sleep_disable(); /* First thing to do is disable sleep. */
  /* Re-enable the peripherals. */
  power_all_enable();
}


//=========================SETUP============================
void setup()
{
  delay(300);
  Serial.begin(9600);

  //светодиод для проверки состояния ардуины
  pinMode(LED_PIN, OUTPUT);

  //=========================
//SLEEP PARAMETERS
   /*** Configure the timer.***/
  /* Normal timer operation.*/
  TCCR1A = 0x00;
  /* Clear the timer counter register.
     You can pre-load this register with a value in order to
     reduce the timeout period, say if you wanted to wake up
     ever 4.0 seconds exactly.
  */
  TCNT1 = 0x0000;
  /* Configure the prescaler for 1:1024, giving us a
     timeout of 4.09 seconds.
  */
  TCCR1B = 0x05;
  /* Enable the timer overlow interrupt. */
  TIMSK1 = 0x01; /*** Configure the timer.***/
  /* Normal timer operation.*/
  TCCR1A = 0x00;
  /* Clear the timer counter register.
     You can pre-load this register with a value in order to
     reduce the timeout period, say if you wanted to wake up
     ever 4.0 seconds exactly.
  */
  TCNT1 = 0x0000;
  /* Configure the prescaler for 1:1024, giving us a
     timeout of 4.09 seconds.
  */
  TCCR1B = 0x05;
  /* Enable the timer overlow interrupt. */
  TIMSK1 = 0x01;

//===============================


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
  if (f_timer == 1)
  {
    f_timer = 0;

    
  // если проверяем температуру
  if (temperatureIsActive_DHT11) {
    //если прошло достаточно времени, считываем данные с датчика температуры
    //if (cTemp.needToCheck(temperatureDelay)) {
      
      sendSensor(makeSensor(controllerNumber, nTemperature, getTemperatureDHT11()));
      delay(50);
    //}
  }

  if (humidityIsActive_DHT11) {
    //если прошло достаточно времени, считываем данные с датчика температуры
   // if (cHum.needToCheck(humidityDelay)) {
      //sendData(keyHumidity, getHumidityDHT11());
      sendSensor(makeSensor(controllerNumber, nHumidity, getHumidityDHT11()));
      delay(50);
   // }
  }

  // если проверяем температуру
  if (temperatureIsActive_DHT22) {
    //если прошло достаточно времени, считываем данные с датчика температуры
    //if (cTemp.needToCheck(temperatureDelay)) {
      
      sendSensor(makeSensor(controllerNumber, nTemperature, getTemperatureDHT22()));
      delay(50);
    //}
  }

  if (humidityIsActive_DHT22) {
    //если прошло достаточно времени, считываем данные с датчика температуры
   // if (cHum.needToCheck(humidityDelay)) {
      
      sendSensor(makeSensor(controllerNumber, nHumidity, getHumidityDHT22()));
      delay(50);
   // }
  }

  if (soilIsActive) {
    //если прошло достаточно времени, считываем данные с датчика температуры
    //if (cSoil.needToCheck(soilDelay)) {
      
      sendSensor(makeSensor(controllerNumber, nSoil, getSoil()));
      delay(50);
    //}
  }

  if (lightIsActive) {
    //если прошло достаточно времени, считываем данные с датчика температуры
    //if (cLight.needToCheck(lightDelay)) {
      
      sendSensor(makeSensor(controllerNumber, nLight, getLight()));
      delay(50);
    //}
  }

  if (photoIsActive) {
    //если прошло достаточно времени, считываем данные с датчика температуры
    //if (cPhoto.needToCheck(photoDelay)) {
      sendSensor(makeSensor(controllerNumber, nPhoto, getPhoto()));
      delay(50);
    //}
  }
  //delay(100);
  enterSleep();
  //delay(1000);
  }

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
  String s = String(data.controllerNumber) + ";" + String(data.key) + ";" + String(data.value);
  Serial.println(s);
}

