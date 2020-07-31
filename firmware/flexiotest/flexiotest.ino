//send out clocks
#include <pindefs.h>
#include <flexiodmaisr.h>
void setup() {
  dac.begin();
  delay(2000);
  dac.setvoltage(1.7);
  setupflexio(1000);
}

void loop() {
  // put your main code here, to run repeatedly:
}
