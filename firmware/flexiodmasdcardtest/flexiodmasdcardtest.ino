//send out clocks
#include <pindefs.h>
#include <flexiodmaisr.h>
#include <AD7866parsing.h>
#include "SdFat.h"
/*********** SETUP SDCARD *****************/
const uint8_t SD_CS_PIN = SS;
SdFs sd;
FsFile file;

void errorHalt(const char* msg) {
  Serial.print("Error: ");
  Serial.println(msg);
  if (sd.sdErrorCode()) {
    if (sd.sdErrorCode() == SD_CARD_ERROR_ACMD41) {
      Serial.println("Try power cycling the SD card.");
    }
    printSdErrorSymbol(&Serial, sd.sdErrorCode());
    Serial.print(", ErrorData: 0X");
    Serial.println(sd.sdErrorData(), HEX);
  }
  while (true) {
    rgb.R(true);
    delay(500);
    rgb.R(false);
    delay(500);
  }
}

#define PRREG(x) Serial.print(#x" 0x"); Serial.println(x,HEX)
void setup() {
  Serial.begin(9600);
  while (!Serial);
  delay(1000);
  rgb.begin();
  pinMode(Arangepin,OUTPUT);
  digitalWrite(Arangepin,HIGH);// set ADC range to 0-5v
  pinMode(A6a0pin,OUTPUT);
  digitalWrite(A6a0pin,HIGH);// set ADC6 to channel 2
  if (!sd.begin(SdioConfig(FIFO_SDIO))) {
    errorHalt("begin failed");
  }
  
  delay(1000);
}

void loop() {
  // put your main code here, to run repeatedly:
  //wait until there is new data in frame buffer.
  //Serial.println((framebufferwritepointer-framebufferreadpointer)%framebuffersize);
  int num32frames= 10000;
  /** initializes sd card file for recording.*/
  if (!file.open("recording.bin", O_RDWR | O_CREAT)) {
    errorHalt("open failed");
  }
//  const uint64_t PREALLOCATE_SIZE= 1ULL<<38ULL;
//  if (!file.preAllocate(PREALLOCATE_SIZE)) {
//    //errorHalt("preAllocate failed");
//  }
  if (!file.truncate(0)) {
    errorHalt("truncate failed");
  }
  setupflexiodma();
  setupflexio(40000);
  int maxtime = 0;
  for(int i=0; i<num32frames; i++){
    while( framefifo::framestoread() < 64);
    //write 32 frames
    Frame chunk[frames_per_chunk];
    for(int i=0; i<frames_per_chunk; i++){
        chunk[i]= framefifo::pop();
    }
    uint32_t t = micros();
    /** Write to sd card file.*/
    if (framechunksize != file.write((void*)(&chunk[0]), framechunksize)) {
        errorHalt("write failed");
    }
    t = micros() - t;
    if(t>maxtime) maxtime = t;
  }
  //first turn off flexio, this disables DMA requests
  closeflexio();

  delay(1000);
  Serial.printf("max write time: %d\n",maxtime);
  Serial.printf("framecount: %d\n",framecount);
  Serial.printf("skippedframe: %d\n",skippedframes);
  Serial.println("The next reg should be 0x0 if the DMA kept up with the data");
  PRREG(IMXRT_FLEXIO2_S.SHIFTERR);
  file.rewind();
  for (int frm=0; frm<(1); frm++){
    uint32_t framenum;
    if (4 != file.read(&framenum, 4)) {
          errorHalt("read failed");
    }
    Serial.printf("framenum: %d\n",framenum);
    uint32_t adcdata[10][6];
    if (10*6*4 != file.read(adcdata, 10*6*4)) {
          errorHalt("read failed");
    }
    for(int i=0; i<10; i++){
      for(int j=0; j<6; j++){
        double vA;
        double vB;
        bool junk;
        double junk2;
        AD7866parser(adcdata[i][j], vA, vB, junk2, junk);
        Serial.printf("%f, %f, ",vB,vA);
      }
      Serial.println();
    }
    uint32_t userdata0;
    if (4 != file.read(&userdata0, 4)) {
          errorHalt("read failed");
    }
    Serial.printf("userdata0: %lu\n",userdata0);
    uint32_t userdata1;
    if (4 != file.read(&userdata1, 4)) {
          errorHalt("read failed");
    }
    Serial.printf("userdata1: %lu\n",userdata1);
    uint32_t crc;
    if (4 != file.read(&crc, 4)) {
          errorHalt("read failed");
    }
    Serial.printf("crc: %lu\n",crc);
    
  }
  //then close file
  if (!file.close()) {
    errorHalt("file close failed");
  }
  Serial.println("safe to remove file");
  while(1);//done
}
