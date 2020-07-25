/** DACx0501.h
 * DACx0501 class bit bang spi version
 */

#pragma once

#include <assert.h>     /* assert */
#include <Arduino.h>

/**
 Class to output to DACx0501 using bit bang spi.
*/
class DACx0501 {
  public:
    int syncpin;
    int sclkpin;
    int sdinpin;

    DACx0501(int _syncpin, int _sclkpin, int _sdinpin):
      syncpin(_syncpin),
      sclkpin(_sclkpin),
      sdinpin(_sdinpin){}

    void begin(){
      pinMode(syncpin,OUTPUT);
      digitalWrite(syncpin,HIGH);
      pinMode(sclkpin,OUTPUT);
      digitalWrite(sclkpin,HIGH);
      pinMode(sdinpin,OUTPUT);
      digitalWrite(sdinpin,LOW);
    }

    /**Set the output voltage. */
    void setvoltage(float out){
      //sanitize NaN as 0
      if(isnan(out)) out= 0.0;
      uint16_t data= (uint16_t)(out*(1.0f/5.0f*65536.0f)+0.5f);
      if(out<=0) data=0;
      if(out>=5) data=0xFFFF;
      uint8_t reg= 0x08;
      //bit bang spi
      digitalWrite(syncpin,LOW);
      for (int i=7; i>=0; i--){
        digitalWrite(sdinpin,(reg>>i)&1);
        delayMicroseconds(1);
        digitalWrite(sclkpin,HIGH);
        delayMicroseconds(1);
        digitalWrite(sclkpin,LOW);
        delayMicroseconds(1);
      }
      for (int i=15; i>=0; i--){
        digitalWrite(sdinpin,(data>>i)&1);
        delayMicroseconds(1);
        digitalWrite(sclkpin,HIGH);
        delayMicroseconds(1);
        digitalWrite(sclkpin,LOW);
        delayMicroseconds(1);
      }
      digitalWrite(syncpin,HIGH);
      delayMicroseconds(1);
      digitalWrite(sclkpin,HIGH);
      digitalWrite(sdinpin,LOW);
    }

};
