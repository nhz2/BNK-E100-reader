/** flexiodmaisr.h
 * flexiodmaisr
 */

#pragma once

#include <assert.h>     /* assert */
#include <Arduino.h>
#include <FastCRC.h>
#include <DMAChannel.h>
#include "pindefs.h"

/**
Setup flexio at freq in Hz greater than 250.
returns the real frequency in Hz of reading a frame
*/
double setupflexio(double freq){
  //Turn on clock gating
  CCM_CCGR3 |= CCM_CCGR3_FLEXIO2(CCM_CCGR_ON);
  //Set divide for flexio2/3 core clock
  CCM_CS1CDR &= ~(CCM_CS1CDR_FLEXIO2_CLK_PODF(7) | CCM_CS1CDR_FLEXIO2_CLK_PRED(7)); // clear
  CCM_CS1CDR |= CCM_CS1CDR_FLEXIO2_CLK_PODF(1) | CCM_CS1CDR_FLEXIO2_CLK_PRED(0); //fastest is  1 0, slowest is 7 7.
  //this is the core flexio2/3 core freq
  int flexhz = 480'000'000 / 2 / 1;   // 480MHz/2= 240MHz

  int Asclkperiod = 12;// number of flexio clks in a Asclk period
  //clear shifter overflow errors
  IMXRT_FLEXIO2_S.SHIFTERR = 0xFF;
  //SCLK adc
  IMXRT_FLEXIO2_S.TIMCMP[0] = ((32*2-1)<<8)|(Asclkperiod/2-1);
  IMXRT_FLEXIO2_S.TIMCFG[0] = FLEXIO_TIMCFG_TIMOUT(3) | //Timer Output: 0 - Timer output is logic one when enabled and is not affected by timer reset. 1 - Timer output is logic zero when enabled and is not affected by timer reset. 2 - Timer output is logic one when enabled and on timer reset. 3 - Timer output is logic zero when enabled and on timer reset.
                              FLEXIO_TIMCFG_TIMDEC(0) | //Timer Decrement and shift clock: 0 - Dec on FlexIO clock, Shift clock is Timer output. 1 - Dec on Trigger input (both edges), Shift clock is Timer output. 2 - Dec on Pin input (both edges), Shift clock is Pin input. 3 - Dec on Trigger input (both edges), Shift clock is Trigger input.
                              FLEXIO_TIMCFG_TIMRST(0) | //Timer Reset: 0 - never. 2 - pin = out. 3 - trigger = out. 4 - pin rising edge. 6 - trigger rising edge. 7 - trigger rising or falling edge.
                              FLEXIO_TIMCFG_TIMDIS(2) | //Timer Disable: 0 - never. 1 - timer N-1 disable. 2 - compare. 3 - compare and trigger low. 4 - pin rising or falling edge. 5 - 4 but also triggger must be high. 6 - trigger falling edge
                              FLEXIO_TIMCFG_TIMENA(6) | //Timer Enable: 0 - always. 1 - timer N-1 enable. 2 - trigger high. 3 - trigger high and pin high. 4 - pin rising edge. 5 - pin rising edge and trigger high. 6 - trigger rising edge. 7 - trigger rising or faling edge.
                              FLEXIO_TIMCFG_TSTOP(0) | //Timer Stop Bit enabled on: 0 - never. 1 - compare. 2 - disable. 3 - both.
                              //FLEXIO_TIMCFG_TSTART | //Timer Start Bit enable.
                              0;
  IMXRT_FLEXIO2_S.TIMCTL[0] = FLEXIO_TIMCTL_TRGSEL(4*2+3) | //Trigger Select: when FLEXIO_TIMCTL_TRGSRC, 4*N - Pin 2*N input. 4*N+1 - Shifter N status flag. 4*N+2 - Pin 2*N+1 input. 4*N+3 - Timer N output.
                              //FLEXIO_TIMCTL_TRGPOL | //Trigger Polarity
                              FLEXIO_TIMCTL_TRGSRC | //Trigger Source
                              FLEXIO_TIMCTL_PINCFG(3) | //Timer Pin Configuration: 0 - output disabled. 1 - open drain or bidirectional output enable. 2 - bidirectional output data. 3 - output.
                              FLEXIO_TIMCTL_PINSEL(1) | //Timer Pin Select
                              FLEXIO_TIMCTL_PINPOL | //Timer Pin Polarity
                              FLEXIO_TIMCTL_TIMOD(1) | //Timer Mode: 0 - Timer Disabled. 1 - Dual 8-bit counters baud mode. 2 - Dual 8-bit counters PWM high mode. 3 - Single 16-bit counter mode.
                              0;

  double readfreq = freq*10.0L;
  int cslowflexcount= 32*Asclkperiod; //how many flexio clks to hold Acs low
  double csflexperiod= flexhz/readfreq; //about how many flexio clks should be in a Acs period
  double cshighflexcount= csflexperiod-cslowflexcount; //about how many flexio clks to hold Acs high
  int csclockdiv= 12; //must be a factor of 32*Asclkperiod, as small as possible to get precission
  int cslowcount= cslowflexcount/csclockdiv;
  int cshighcount= cshighflexcount/csclockdiv+0.5L; // (1 to 256)
  while(cshighcount>256 && cslowcount>1){//increase clock div for low freq
    csclockdiv= csclockdiv*2;
    cslowcount= cslowflexcount/csclockdiv;
    cshighcount= cshighflexcount/csclockdiv+0.5L; // (1 to 256)
  }
  if (cshighcount>256) cshighcount= 256;
  if (cshighcount<3) cshighcount= 3;
  int csperiod= cslowcount+cshighcount;
  int realcsflexperiod= csperiod*csclockdiv;// less than 98688 must be a mult of 4
  assert ((realcsflexperiod/4)*4 == realcsflexperiod);

  //CS clock divider
  IMXRT_FLEXIO2_S.TIMCMP[1] = csclockdiv-1;
  IMXRT_FLEXIO2_S.TIMCFG[1] = FLEXIO_TIMCFG_TIMOUT(0) | //Timer Output: 0 - Timer output is logic one when enabled and is not affected by timer reset. 1 - Timer output is logic zero when enabled and is not affected by timer reset. 2 - Timer output is logic one when enabled and on timer reset. 3 - Timer output is logic zero when enabled and on timer reset.
                              FLEXIO_TIMCFG_TIMDEC(0) | //Timer Decrement and shift clock: 0 - Dec on FlexIO clock, Shift clock is Timer output. 1 - Dec on Trigger input (both edges), Shift clock is Timer output. 2 - Dec on Pin input (both edges), Shift clock is Pin input. 3 - Dec on Trigger input (both edges), Shift clock is Trigger input.
                              FLEXIO_TIMCFG_TIMRST(0) | //Timer Reset: 0 - never. 2 - pin = out. 3 - trigger = out. 4 - pin rising edge. 6 - trigger rising edge. 7 - trigger rising or falling edge.
                              FLEXIO_TIMCFG_TIMDIS(0) | //Timer Disable: 0 - never. 1 - timer N-1 disable. 2 - compare. 3 - compare and trigger low. 4 - pin rising or falling edge. 5 - 4 but also triggger must be high. 6 - trigger falling edge
                              FLEXIO_TIMCFG_TIMENA(0) | //Timer Enable: 0 - always. 1 - timer N-1 enable. 2 - trigger high. 3 - trigger high and pin high. 4 - pin rising edge. 5 - pin rising edge and trigger high. 6 - trigger rising edge. 7 - trigger rising or faling edge.
                              FLEXIO_TIMCFG_TSTOP(0) | //Timer Stop Bit enabled on: 0 - never. 1 - compare. 2 - disable. 3 - both.
                              //FLEXIO_TIMCFG_TSTART | //Timer Start Bit enable.
                              0;
  IMXRT_FLEXIO2_S.TIMCTL[1] = FLEXIO_TIMCTL_TRGSEL(0) | //Trigger Select: when FLEXIO_TIMCTL_TRGSRC, 4*N - Pin 2*N input. 4*N+1 - Shifter N status flag. 4*N+2 - Pin 2*N+1 input. 4*N+3 - Timer N output.
                              //FLEXIO_TIMCTL_TRGPOL | //Trigger Polarity
                              FLEXIO_TIMCTL_TRGSRC | //Trigger Source
                              FLEXIO_TIMCTL_PINCFG(0) | //Timer Pin Configuration: 0 - output disabled. 1 - open drain or bidirectional output enable. 2 - bidirectional output data. 3 - output.
                              FLEXIO_TIMCTL_PINSEL(0) | //Timer Pin Select
                              //FLEXIO_TIMCTL_PINPOL | //Timer Pin Polarity
                              FLEXIO_TIMCTL_TIMOD(3) | //Timer Mode: 0 - Timer Disabled. 1 - Dual 8-bit counters baud mode. 2 - Dual 8-bit counters PWM high mode. 3 - Single 16-bit counter mode.
                              0;

  //CS
  assert (cslowcount>=1 && cslowcount<=256);
  assert (cshighcount>=1 && cshighcount<=256);
  IMXRT_FLEXIO2_S.TIMCMP[2] = ((cshighcount-1)<<8)|(cslowcount-1);
  IMXRT_FLEXIO2_S.TIMCFG[2] = FLEXIO_TIMCFG_TIMOUT(2) | //Timer Output: 0 - Timer output is logic one when enabled and is not affected by timer reset. 1 - Timer output is logic zero when enabled and is not affected by timer reset. 2 - Timer output is logic one when enabled and on timer reset. 3 - Timer output is logic zero when enabled and on timer reset.
                              FLEXIO_TIMCFG_TIMDEC(1) | //Timer Decrement and shift clock: 0 - Dec on FlexIO clock, Shift clock is Timer output. 1 - Dec on Trigger input (both edges), Shift clock is Timer output. 2 - Dec on Pin input (both edges), Shift clock is Pin input. 3 - Dec on Trigger input (both edges), Shift clock is Trigger input.
                              FLEXIO_TIMCFG_TIMRST(0) | //Timer Reset: 0 - never. 2 - pin = out. 3 - trigger = out. 4 - pin rising edge. 6 - trigger rising edge. 7 - trigger rising or falling edge.
                              FLEXIO_TIMCFG_TIMDIS(0) | //Timer Disable: 0 - never. 1 - timer N-1 disable. 2 - compare. 3 - compare and trigger low. 4 - pin rising or falling edge. 5 - 4 but also triggger must be high. 6 - trigger falling edge
                              FLEXIO_TIMCFG_TIMENA(0) | //Timer Enable: 0 - always. 1 - timer N-1 enable. 2 - trigger high. 3 - trigger high and pin high. 4 - pin rising edge. 5 - pin rising edge and trigger high. 6 - trigger rising edge. 7 - trigger rising or faling edge.
                              FLEXIO_TIMCFG_TSTOP(0) | //Timer Stop Bit enabled on: 0 - never. 1 - compare. 2 - disable. 3 - both.
                              //FLEXIO_TIMCFG_TSTART | //Timer Start Bit enable.
                              0;
  IMXRT_FLEXIO2_S.TIMCTL[2] = FLEXIO_TIMCTL_TRGSEL(4*1+3) | //Trigger Select: when FLEXIO_TIMCTL_TRGSRC, 4*N - Pin 2*N input. 4*N+1 - Shifter N status flag. 4*N+2 - Pin 2*N+1 input. 4*N+3 - Timer N output.
                              //FLEXIO_TIMCTL_TRGPOL | //Trigger Polarity
                              FLEXIO_TIMCTL_TRGSRC | //Trigger Source
                              FLEXIO_TIMCTL_PINCFG(3) | //Timer Pin Configuration: 0 - output disabled. 1 - open drain or bidirectional output enable. 2 - bidirectional output data. 3 - output.
                              FLEXIO_TIMCTL_PINSEL(2) | //Timer Pin Select
                              FLEXIO_TIMCTL_PINPOL | //Timer Pin Polarity
                              FLEXIO_TIMCTL_TIMOD(2) | //Timer Mode: 0 - Timer Disabled. 1 - Dual 8-bit counters baud mode. 2 - Dual 8-bit counters PWM high mode. 3 - Single 16-bit counter mode.
                              0;

  //Setup input This is on flexio2 which has dma, it is in spi recieve mode
  int flexio2pinsAout[6]={10,17,16,11,0,12};
  for (int i=0; i<6; i++){
    IMXRT_FLEXIO2_S.SHIFTCFG[i]= FLEXIO_SHIFTCFG_PWIDTH(0) | //Parallel Width
                                 //FLEXIO_SHIFTCFG_INSRC | //Input Source 0b - Pin. 1b - Shifter N+1 Output
                                 FLEXIO_SHIFTCFG_SSTOP(0) |
                                 FLEXIO_SHIFTCFG_SSTART(0) |
                                 //TODO a bunch of stuff about start and stop bits.
                                 0;
    //read input data from pin 1 aka teensy pin 12 on neg edge of timer 0
    IMXRT_FLEXIO2_S.SHIFTCTL[i]= FLEXIO_SHIFTCTL_TIMSEL(0) | //Selects which Timer is used for controlling the logic/shift register and generating the Shift clock. TIMSEL=i will select TIMERi.
                                 //FLEXIO_SHIFTCTL_TIMPOL    | //Timer Polarity 0b - Shift on posedge of Shift clock 1b - Shift on negedge of Shift clock
                                 FLEXIO_SHIFTCTL_PINCFG(0) | //Shifter Pin Configuration 00b - Shifter pin output disabled 01b - Shifter pin open drain or bidirectional output enable 10b - Shifter pin bidirectional output data 11b - Shifter pin output
                                 FLEXIO_SHIFTCTL_PINSEL(flexio2pinsAout[i]) | //Shifter Pin Select Selects which pin is used by the Shifter input or output. PINSEL=i will select the FXIO_Di pin.
                                 //FLEXIO_SHIFTCTL_PINPOL | //Shifter Pin Polarity 0b - Pin is active high 1b - Pin is active low
                                 FLEXIO_SHIFTCTL_SMOD(1) | //Shifter Mode. 0 - Disabled. 1 - Receive mode. 2 - Transmit mode. 3 - Reserved. 4 - Match Store mode. 5 - Match Continuous mode. 6 - State mode. 7 - Logic mode.
                                 0;
    //IMXRT_FLEXIO2_S.SHIFTBUF[i]= 0;//32 bits of data to read
  }
  // enable dma on shifter 0
  IMXRT_FLEXIO2_S.SHIFTSDEN = 1;

  //CHIP CLK
  //This is on a seperate flexio to save pins for flexio2. It should be on the same clock though.
  IMXRT_FLEXIO3_S.TIMCMP[0] = realcsflexperiod/4-1;
  IMXRT_FLEXIO3_S.TIMCFG[0] = FLEXIO_TIMCFG_TIMOUT(0) | //Timer Output: 0 - Timer output is logic one when enabled and is not affected by timer reset. 1 - Timer output is logic zero when enabled and is not affected by timer reset. 2 - Timer output is logic one when enabled and on timer reset. 3 - Timer output is logic zero when enabled and on timer reset.
                              FLEXIO_TIMCFG_TIMDEC(0) | //Timer Decrement and shift clock: 0 - Dec on FlexIO clock, Shift clock is Timer output. 1 - Dec on Trigger input (both edges), Shift clock is Timer output. 2 - Dec on Pin input (both edges), Shift clock is Pin input. 3 - Dec on Trigger input (both edges), Shift clock is Trigger input.
                              FLEXIO_TIMCFG_TIMRST(0) | //Timer Reset: 0 - never. 2 - pin = out. 3 - trigger = out. 4 - pin rising edge. 6 - trigger rising edge. 7 - trigger rising or falling edge.
                              FLEXIO_TIMCFG_TIMDIS(0) | //Timer Disable: 0 - never. 1 - timer N-1 disable. 2 - compare. 3 - compare and trigger low. 4 - pin rising or falling edge. 5 - 4 but also triggger must be high. 6 - trigger falling edge
                              FLEXIO_TIMCFG_TIMENA(0) | //Timer Enable: 0 - always. 1 - timer N-1 enable. 2 - trigger high. 3 - trigger high and pin high. 4 - pin rising edge. 5 - pin rising edge and trigger high. 6 - trigger rising edge. 7 - trigger rising or faling edge.
                              FLEXIO_TIMCFG_TSTOP(0) | //Timer Stop Bit enabled on: 0 - never. 1 - compare. 2 - disable. 3 - both.
                              //FLEXIO_TIMCFG_TSTART | //Timer Start Bit enable.
                              0;
  IMXRT_FLEXIO3_S.TIMCTL[0] = FLEXIO_TIMCTL_TRGSEL(0) | //Trigger Select: when FLEXIO_TIMCTL_TRGSRC, 4*N - Pin 2*N input. 4*N+1 - Shifter N status flag. 4*N+2 - Pin 2*N+1 input. 4*N+3 - Timer N output.
                              //FLEXIO_TIMCTL_TRGPOL | //Trigger Polarity
                              FLEXIO_TIMCTL_TRGSRC | //Trigger Source
                              FLEXIO_TIMCTL_PINCFG(3) | //Timer Pin Configuration: 0 - output disabled. 1 - open drain or bidirectional output enable. 2 - bidirectional output data. 3 - output.
                              FLEXIO_TIMCTL_PINSEL(6) | //Timer Pin Select
                              FLEXIO_TIMCTL_PINPOL | //Timer Pin Polarity
                              FLEXIO_TIMCTL_TIMOD(3) | //Timer Mode: 0 - Timer Disabled. 1 - Dual 8-bit counters baud mode. 2 - Dual 8-bit counters PWM high mode. 3 - Single 16-bit counter mode.
                              0;

  //CHIP SCLK
  //This is on a seperate flexio to save pins for flexio2. It should be on the same clock though.
  IMXRT_FLEXIO3_S.TIMCMP[1] = ((38-1)<<8)|(2-1);
  IMXRT_FLEXIO3_S.TIMCFG[1] = FLEXIO_TIMCFG_TIMOUT(0) | //Timer Output: 0 - Timer output is logic one when enabled and is not affected by timer reset. 1 - Timer output is logic zero when enabled and is not affected by timer reset. 2 - Timer output is logic one when enabled and on timer reset. 3 - Timer output is logic zero when enabled and on timer reset.
                              FLEXIO_TIMCFG_TIMDEC(1) | //Timer Decrement and shift clock: 0 - Dec on FlexIO clock, Shift clock is Timer output. 1 - Dec on Trigger input (both edges), Shift clock is Timer output. 2 - Dec on Pin input (both edges), Shift clock is Pin input. 3 - Dec on Trigger input (both edges), Shift clock is Trigger input.
                              FLEXIO_TIMCFG_TIMRST(0) | //Timer Reset: 0 - never. 2 - pin = out. 3 - trigger = out. 4 - pin rising edge. 6 - trigger rising edge. 7 - trigger rising or falling edge.
                              FLEXIO_TIMCFG_TIMDIS(0) | //Timer Disable: 0 - never. 1 - timer N-1 disable. 2 - compare. 3 - compare and trigger low. 4 - pin rising or falling edge. 5 - 4 but also triggger must be high. 6 - trigger falling edge
                              FLEXIO_TIMCFG_TIMENA(0) | //Timer Enable: 0 - always. 1 - timer N-1 enable. 2 - trigger high. 3 - trigger high and pin high. 4 - pin rising edge. 5 - pin rising edge and trigger high. 6 - trigger rising edge. 7 - trigger rising or faling edge.
                              FLEXIO_TIMCFG_TSTOP(0) | //Timer Stop Bit enabled on: 0 - never. 1 - compare. 2 - disable. 3 - both.
                              //FLEXIO_TIMCFG_TSTART | //Timer Start Bit enable.
                              0;
  IMXRT_FLEXIO3_S.TIMCTL[1] = FLEXIO_TIMCTL_TRGSEL(4*0+3) | //Trigger Select: when FLEXIO_TIMCTL_TRGSRC, 4*N - Pin 2*N input. 4*N+1 - Shifter N status flag. 4*N+2 - Pin 2*N+1 input. 4*N+3 - Timer N output.
                              //FLEXIO_TIMCTL_TRGPOL | //Trigger Polarity
                              FLEXIO_TIMCTL_TRGSRC | //Trigger Source
                              FLEXIO_TIMCTL_PINCFG(3) | //Timer Pin Configuration: 0 - output disabled. 1 - open drain or bidirectional output enable. 2 - bidirectional output data. 3 - output.
                              FLEXIO_TIMCTL_PINSEL(1) | //Timer Pin Select
                              FLEXIO_TIMCTL_PINPOL | //Timer Pin Polarity
                              FLEXIO_TIMCTL_TIMOD(2) | //Timer Mode: 0 - Timer Disabled. 1 - Dual 8-bit counters baud mode. 2 - Dual 8-bit counters PWM high mode. 3 - Single 16-bit counter mode.
                              0;

  *(portConfigRegister(A1outpin)) = 4; // alt
  *(portConfigRegister(A2outpin)) = 4; // alt
  *(portConfigRegister(A3outpin)) = 4; // alt
  *(portConfigRegister(A4outpin)) = 4; // alt
  *(portConfigRegister(A5outpin)) = 4; // alt
  *(portConfigRegister(A6outpin)) = 4; // alt
  *(portConfigRegister(Acspin)) = 4; // alt
  *(portConfigRegister(Asclkpin)) = 4; // alt
  *(portConfigRegister(Cclkpin)) = 9; // alt
  *(portConfigRegister(Csclkpin)) = 9; // alt

  //phase shift
  double realf= ((double)flexhz)/((double)realcsflexperiod)/10.0L;
  //int delaymicro= 0;//1.0L/realf/20.0L*1E6;
  FLEXIO2_CTRL |= FLEXIO_CTRL_FLEXEN;
  //delayMicroseconds(delaymicro);
  //for (uint8_t i=0;i<26;i++) __asm__ __volatile__ ("nop\n\t");  //Some race condition between clearInterrupt() and the return of the ISR.  If we don't delay here, the ISR will fire again.
  FLEXIO3_CTRL |= FLEXIO_CTRL_FLEXEN;
  return realf;
}

/** disable the flexio.*/
void closeflexio(){
  FLEXIO2_CTRL &= ~FLEXIO_CTRL_FLEXEN;
  FLEXIO3_CTRL &= ~FLEXIO_CTRL_FLEXEN;
}


/************ DMA ISR Globals *******/
const uint32_t framesize = 256;// frame size in bytes
const uint32_t frames_per_chunk= 32;
const uint32_t framechunksize= frames_per_chunk*framesize;
const uint32_t framebuffersize = framechunksize*32; //frame buffer size in bytes
uint8_t framebuffer[framebuffersize];
volatile uint32_t framebufferwritepointer=0;
volatile uint32_t framebufferreadpointer=0;

inline uint32_t numbytesinframebuffer(){
  return   (framebufferwritepointer-framebufferreadpointer)%framebuffersize;
}

volatile int32_t userdata0;
volatile int32_t userdata1;
volatile uint32_t framecount=0;
volatile uint32_t skippedframes=0;
volatile bool firstint;


/************ DMA Buffer and configs *******/
#define DMABUFFERSIZEPOW 16
const int dmabuffersize = 1<<DMABUFFERSIZEPOW; //64 kB buffer
volatile uint8_t dmabuffer[dmabuffersize] __attribute__ ((aligned (1<<DMABUFFERSIZEPOW)));
volatile int dmabufferreadpointer;
const int numframesinterrupt = 64; // number of frames per interrupt
const int numinterrupt = numframesinterrupt*10; // number of dma requests per interrupt
const int numshiftbuf= 6; //number of buffers
DMAChannel dmachannel;


/** DMA interrupt */
void dmaisr();

/**
Setup DMA from FLEXIO2 uses numshiftbuf buffers starting at 0
Do this before enabling flexio2
numinterrupt is the number of transfers before an interrupt of dmaisr
*/
void setupflexiodma(){
  dmachannel.disable();
  framebufferwritepointer= 0;
  framebufferreadpointer= 0;
  framecount= 0;
  skippedframes= 0;
  dmabufferreadpointer= 0;
  firstint= true;
  //Apply settings
  dmachannel.triggerAtHardwareEvent(DMAMUX_SOURCE_FLEXIO2_REQUEST0);
  dmachannel.TCD->SADDR = IMXRT_FLEXIO2_S.SHIFTBUFBIS;//Source Address: Memory address pointing to the source data.
	dmachannel.TCD->SOFF = 4;//Source address signed offset: Sign-extended offset applied to the current source address to form the next-state value as each source read is completed.
	dmachannel.TCD->ATTR = DMA_TCD_ATTR_SMOD(0) | //Source Address Modulo: 0 - disabled. 00001-11111b - The number of lower address bits allowed to change.
                         DMA_TCD_ATTR_SSIZE(DMA_TCD_ATTR_SIZE_32BIT) | //Source data transfer size
                         DMA_TCD_ATTR_DMOD(DMABUFFERSIZEPOW) | //Destination Address Modulo: 0 - disabled. 00001-11111b - The number of lower address bits allowed to change.
                         DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_32BIT) | //Destination data transfer size
                         0;
  dmachannel.TCD->NBYTES_MLOFFYES = DMA_TCD_NBYTES_SMLOE | //Source Minor Loop Offset Enable
                                    DMA_TCD_NBYTES_MLOFFYES_MLOFF(-numshiftbuf*4) | //Minor offset added to S or D address upon minor loop completion. Return to start, not added at major loop
                                    DMA_TCD_NBYTES_MLOFFYES_NBYTES(numshiftbuf*4) | //Minor Byte Transfer Count number of bytes to transfer per request.
                                    0;
	dmachannel.TCD->SLAST = -numshiftbuf*4;//Last Source Address Adjustment: added to the source address at the completion of the major iteration count.
  dmachannel.TCD->DADDR = (volatile uint32_t *)dmabuffer;
	dmachannel.TCD->DOFF = 4;
	dmachannel.TCD->CITER = numinterrupt;//current major loop count, same as BITER on init, then decremented on each minor transfer.
	dmachannel.TCD->DLASTSGA = 0;
	dmachannel.TCD->BITER = numinterrupt;

  dmachannel.attachInterrupt(&dmaisr);
  dmachannel.interruptAtCompletion();
  //dmachannel.interruptAtHalf();

  //finally enable
  dmachannel.enable();
}

FastCRC32 CRC32;

/** Add an offset to the pointer x like the DMA would with mod,
  only affecting mod LSb of the pointer address.*/
uint8_t* addoffset(uint8_t* x, int mod, int offset){
  uint32_t xloc= (uint32_t)x;
  xloc= ( (xloc+offset) & ~(-1<<mod) ) | ( (xloc) & (-1<<mod) );
  return (uint8_t*) xloc;
}

/** Packs 240bytes of rawdata and stores it as 256 bytes of data in destination.

The frame is packed as {framenumber,rawdata,userdata0,userdata1,CRC32} in little endian order.
CRC-32 has Alias CRC-32/ADCCP, PKZIP, Ethernet, 802.3
dmod is the mod used by the DMA to write the data in a circular buffer
*/
void packframe(volatile const uint8_t* rawdata, int mod, volatile uint8_t* destination, uint32_t framenumber, int32_t userdata0, int32_t userdata1){
  //copy memory to destination
  volatile uint8_t* initdest= destination;
  *((uint32_t*)(destination))= framenumber;
  destination+= 4;
  for (int i=0; i<numshiftbuf*4*10; i++){
    *destination= *rawdata;
    destination++;
    rawdata= addoffset(rawdata, mod, 1);
  }
  *((int32_t*)(destination))= userdata0;
  destination+= 4;
  *((int32_t*)(destination))= userdata1;
  destination+= 4;
  *((uint32_t*)(destination))= CRC32.crc32(initdest, 252);
  return;
}


void dmaisr(){
  uint32_t numframes2process= numframesinterrupt;
  if(firstint) {
    //ignore first row of data.
    numframes2process--;
    dmabufferreadpointer += numshiftbuf*4;
    firstint=false;
  }
  //check for over run in frame buffer
  uint32_t frame_buftransize = numframes2process*framesize;
  uint32_t final_framebufferwritepointer= framebufferwritepointer+frame_buftransize;
  if ((final_framebufferwritepointer-framebufferreadpointer)%framebuffersize < (framebufferwritepointer-framebufferreadpointer)%framebuffersize){
    //over run occured, turn on blue led
    rgb.B(true);
    //skip frames
    dmabufferreadpointer += numframes2process*10*numshiftbuf*4;
    dmabufferreadpointer= dmabufferreadpointer%dmabuffersize;
    framecount+= numframes2process;
    skippedframes+= numframes2process;
  } else{
    //write frames to buffer
    for (int i=0; i<numframes2process; i++){
      packframe(&dmabuffer[dmabufferreadpointer],DMABUFFERSIZEPOW,&framebuffer[framebufferwritepointer],framecount,userdata0,userdata1);
      dmabufferreadpointer += 10*numshiftbuf*4;
      dmabufferreadpointer= dmabufferreadpointer%dmabuffersize;
      framebufferwritepointer= (framebufferwritepointer+framesize)%framebuffersize;
      framecount++;
    }
  }
  //the following us from https://github.com/wramsdell/TriantaduoWS2811 of how to end a dma isr
  /* Clear the interrupt so we don't get triggered again */
  dmachannel.clearInterrupt();

  /* Spin for a few cycles.  If we don't do this, the interrupt doesn't clear and we get triggered a second time */
  for (uint8_t i=0;i<10;i++) __asm__ __volatile__ ("nop\n\t");  //Some race condition between clearInterrupt() and the return of the ISR.  If we don't delay here, the ISR will fire again.
}
