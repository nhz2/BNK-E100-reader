//send out clocks
#include <pindefs.h>
#include <filestuff.h>
#define PRREG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)
void setup() {
  Serial.begin(9600);
  while (!Serial);
  delay(1000);
  pinMode(RGBpwrpin,OUTPUT);
  digitalWrite(RGBpwrpin,HIGH);
  pinMode(Rpin,OUTPUT);
  digitalWrite(Rpin,HIGH);
  pinMode(Gpin,OUTPUT);
  digitalWrite(Gpin,HIGH);
  pinMode(Bpin,OUTPUT);
  digitalWrite(Bpin,HIGH);
  setupflexiodma();
  setupflexio(40000);
  PRREG(IMXRT_FLEXIO2_S.SHIFTSTAT);
  Serial.printf("%p\n",bigbuffer);
  
}

void loop() {
  // put your main code here, to run repeatedly:
//  while(IMXRT_FLEXIO2_S.SHIFTSTAT==0);
//  for( int i=0; i<6; i++){
//    PRREG(IMXRT_FLEXIO2_S.SHIFTBUFBIS[i]);
//    digitalWrite(13,LOW);
//  }
  delay(100);
  Serial.println(dmachannel.TCD->CITER);
  PRREG(IMXRT_DMA.CR);
  PRREG(IMXRT_DMA.ERQ);
  PRREG(IMXRT_DMA.HRS);
  PRREG(IMXRT_FLEXIO2_S.SHIFTERR);
  Serial.printf("%p\n",dmachannel.TCD->SADDR);
  Serial.printf("%p\n",dmachannel.TCD->DADDR);
  Serial.printf("%d\n",bigbuffer[1]);
  
}
