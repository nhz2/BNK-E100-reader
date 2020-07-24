//blink rgb led
#include <pindefs.h>
void setup() {
  rgb.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  rgb.R(true);
  delay(200);
  rgb.R(false);
  delay(200);
  rgb.G(true);
  delay(200);
  rgb.G(false);
  delay(200);
  rgb.B(true);
  delay(200);
  rgb.B(false);
  delay(1000);
}
