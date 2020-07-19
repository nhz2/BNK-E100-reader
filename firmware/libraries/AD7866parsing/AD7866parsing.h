/** AD7866parsing.h
 utility functions to parse AD7866 data
*/

#pragma once

/** Gets the voltages assuming Range stays constant and Vref is 2.5V
Returns false and sets to NAN if not valid*/
inline bool AD7866parser(uint32_t data,
  double& voltageA,
  double& voltageB,
  double& maxvoltage,
  bool& channel){
  {
    uint32_t leadingzeroH = (data>>31)&1;
    uint32_t leadingzeroL = (data>>15)&1;
    if(leadingzeroH!=0 || leadingzeroL!=0){
      Serial.println("leading zeros not there");
      goto INVALID;
    }
    uint32_t rangeH = (data>>30)&1;
    uint32_t rangeL = (data>>14)&1;
    if(rangeH!= rangeL){
      Serial.println("range not equal");
      goto INVALID;
    }
    uint32_t channelH = (data>>29)&1;
    uint32_t channelL = (data>>13)&1;
    if(channelH != channelL){
      Serial.println("channel not equal");
      goto INVALID;
    }
    uint32_t BboolH = (data>>28)&1;
    uint32_t BboolL = (data>>12)&1;
    if(BboolH == BboolL){
      Serial.println("only one B");
      goto INVALID;
    }
    uint32_t dataH = (data>>16)&0xFFF;
    uint32_t dataL = (data)&0xFFF;
    double vH;
    double vL;
    if(rangeH==0){
      //0v to Vref, straight binary
      vH= ((double)dataH)*(2.5L/4096.0L);
      vL= ((double)dataL)*(2.5L/4096.0L);
      maxvoltage= ((double)0xFFF)*(2.5L/4096.0L);
    }else{
      //2s complement Vref+-Vref
      //shift back to straight binary
      dataH^= 1<<11;
      dataL^= 1<<11;
      vH= ((double)dataH)*(5.0L/4096.0L);
      vL= ((double)dataL)*(5.0L/4096.0L);
      maxvoltage= ((double)0xFFF)*(5.0L/4096.0L);
    }
    if(BboolH){
      voltageB= vH;
      voltageA= vL;
    } else {
      voltageA= vH;
      voltageB= vL;
    }
    return true;
  }
  INVALID:
  {
    voltageA= NAN;
    voltageB= NAN;
    maxvoltage= NAN;
    channel= false;
    return false;
  }
}
