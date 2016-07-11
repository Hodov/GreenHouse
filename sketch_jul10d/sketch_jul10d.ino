bool sender = false;

#include <SPI.h>
#include <Arduino.h>
#include <RF24.h>
#include <nRF24L01.h>

byte addresses[][6] = {"00001"};

struct Sensor {
  int controllerNumber;
  int key;
  int value;
};

RF24 radio(7, 8);

const byte mainControllerAddr[6] = "00001";

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  // модем работает на отправку
  radio.begin();
  if (sender) {
    radio.openWritingPipe(addresses[0]);
  } else {
    radio.openReadingPipe(1, addresses[0]);
  }

  radio.startListening();
}

void loop() {
  Sensor output;

  // put your main code here, to run repeatedly:
  int outputData2 = 1;

  if (sender) {
    radio.stopListening();
    output.controllerNumber = 1;
    output.key = 2;
    output.value = 22;

    Serial.println(output.key);
    
    if (!radio.write( &output, sizeof(output) )) {
      Serial.println(F("failed"));
    } else {
      Serial.println(F("send"));
    }
    radio.startListening();
  } else {
    int b;
    Sensor input;
    if (radio.available()) {

      radio.read( &input, sizeof(input) );
    }

    Serial.println(input.key);
  }
  delay(1000);

}
