int rele = 12;

void setup() {
  // put your setup code here, to run once:
  
  //перечисление реле
  pinMode(rele, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  
  digitalWrite(rele, HIGH);
  delay(10000);
  digitalWrite(rele, LOW);
  delay(10000);
}
