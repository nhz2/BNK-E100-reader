//send out clocks
#include <pindefs.h>

void setup() {
  dac.begin();
}

void loop() {
  dac.setvoltage(1.0);
}
