/** RGBLED.h
 * RGBLED class
 */

#pragma once

#include <Arduino.h>

/**
 Class to output to RGBLED with common anode.
*/
class RGBLED {
  public:
    int anodepin;
    int rpin;
    int gpin;
    int bpin;

    RGBLED(int _anodepin, int _rpin, int _gpin, int _bpin):
      anodepin(_anodepin),
      rpin(_rpin),
      gpin(_gpin),
      bpin(_bpin){}

    void begin(){
      pinMode(rpin,OUTPUT);
      digitalWrite(rpin,HIGH);
      pinMode(gpin,OUTPUT);
      digitalWrite(gpin,HIGH);
      pinMode(bpin,OUTPUT);
      digitalWrite(bpin,HIGH);
      pinMode(anodepin,OUTPUT);
      digitalWrite(anodepin,HIGH);
    }

    /**set red LED on state. */
    void R(bool on){
      digitalWrite(rpin,!on);
    }

    /**set green LED on state. */
    void G(bool on){
      digitalWrite(gpin,!on);
    }

    /**set blue LED on state. */
    void B(bool on){
      digitalWrite(bpin,!on);
    }

};
