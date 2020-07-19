//send out clocks
#include <pindefs.h>
#include <DACx0501.h>

DACx0501 dac(Dsyncpin,Dsclkpin,Dsdinpin);

void setup() {
  dac.begin();
}

void loop() {
  dac.setvoltage(1.0);
}
