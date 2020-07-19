//blink rgb led
#include <pindefs.h>
void setup() {
  pinMode(RGBpwrpin,OUTPUT);
  digitalWrite(RGBpwrpin,HIGH);
  pinMode(Rpin,OUTPUT);
  pinMode(Gpin,OUTPUT);
  pinMode(Bpin,OUTPUT);
  digitalWrite(Rpin,HIGH);
  digitalWrite(Gpin,HIGH);
  digitalWrite(Bpin,HIGH);

}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(Rpin,LOW);
  delay(200);
  digitalWrite(Rpin,HIGH);
  delay(200);
  digitalWrite(Gpin,LOW);
  delay(200);
  digitalWrite(Gpin,HIGH);
  delay(200);
  digitalWrite(Bpin,LOW);
  delay(200);
  digitalWrite(Bpin,HIGH);
  delay(1000);

}
