//send out clocks
#include <pindefs.h>
#include <flexiodmaisr.h>
void setup() {
  rgb.begin();
  setupflexio(1000);
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
