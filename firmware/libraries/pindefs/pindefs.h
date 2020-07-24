/** pindefs.h
  * The pin definitions.
  * See https://easyeda.com/nzimmerberg/amp-chip for a schematic.
  */

#pragma once

#include <assert.h>     /* assert */
#include <Arduino.h>


//Name and Teensy 4.1 pin number //teensy 4.1 pin name
const int Rpin = 31;// EMC-36
const int Gpin = 30;// EMC-37
const int Bpin = 29;// EMC-31
const int RGBpwrpin = 33;// EMC-07 //FLEXIO1-D07 ALT4
const int Csclkpin = 18;// AD-B1-01 //FLEXIO3-D01 ALT9
const int Cclkpin = 17;// AD-B1-06 //FLEXIO3-D06 ALT9
const int Dsdinpin = 16;// AD-B1-07 //FLEXIO3-D07 ALT9
const int Dsyncpin = 15;// AD-B1-03 //FLEXIO3-D03 ALT9
const int Dsclkpin = 14;// AD-B1-02 //FLEXIO3-D02 ALT9
const int A6a0pin = 37;// B1-03 //FLEXIO2-D19 ALT4 //FLEXIO3-D19 ALT9
const int A6outpin = 32;// B0-12 //FLEXIO2-D12 ALT4
const int Asclkpin = 12;// B0-01 //FLEXIO2-D01 ALT4
const int Acspin = 11;// B0-02 //FLEXIO2-D02 ALT4
const int A5outpin = 10;// B0-00 //FLEXIO2-D00 ALT4
const int A4outpin = 9;// B0-11 //FLEXIO2-D11 ALT4
const int A3outpin = 8;// B1-00 //FLEXIO2-D16 ALT4 //FLEXIO3-D16 ALT9
const int A2outpin = 7;// B1-01 //FLEXIO2-D17 ALT4 //FLEXIO3-D17 ALT9
const int A1outpin = 6;// B0-10 //FLEXIO2-D10 ALT4
const int Arangepin = 3;// EMC-05 //FLEXIO1-D05 ALT4
