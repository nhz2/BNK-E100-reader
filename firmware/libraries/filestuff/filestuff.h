/** filestuff.h
 * file functions
 */

#pragma once

#include <assert.h>     /* assert */
#include <Arduino.h>
#include <FastCRC.h>
#include <DMAChannel.h>
#include "pindefs.h"




/************ DMA ISR Globals *******/
const uint32_t framebuffersize = 1<<18; //256 kB buffer
const uint32_t framesize = 256;// frame size in bytes
volatile uint8_t framebuffer[framebuffersize];
volatile uint32_t framebufferwritepointer;
volatile uint32_t framebufferreadpointer;
volatile uint32_t framecount;
volatile uint32_t skippedframes;
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

The frame is packed as {framenumber,rawdata,userdata0,userdata1,CRC32}
CRC-32 has Alias CRC-32/ADCCP, PKZIP, Ethernet, 802.3
dmod is the mod used by the DMA to write the data in a circular buffer
*/
void packframe(volatile const uint8_t* rawdata, int mod, volatile uint8_t* destination, uint32_t framenumber, uint32_t userdata0, uint32_t userdata1){
  //copy memory to destination
  volatile uint8_t* initdest= destination;
  *((uint32_t*)(destination))= framenumber;
  destination+= 4;
  for (int i=0; i<numshiftbuf*4*10; i++){
    *destination= *rawdata;
    destination++;
    rawdata= addoffset(rawdata, mod, 1);
  }
  *((uint32_t*)(destination))= userdata0;
  destination+= 4;
  *((uint32_t*)(destination))= userdata1;
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
    digitalWrite(Bpin,LOW);
    //skip frames
    dmabufferreadpointer += numframes2process*10*numshiftbuf*4;
    dmabufferreadpointer= dmabufferreadpointer%dmabuffersize;
    framecount+= numframes2process;
    skippedframes+= numframes2process;
  } else{
    //write frames to buffer
    for (int i=0; i<numframes2process; i++){
      packframe(&dmabuffer[dmabufferreadpointer],DMABUFFERSIZEPOW,&framebuffer[framebufferwritepointer],framecount,0,0);
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
