//send out clocks
#include <pindefs.h>
#include <filestuff.h>
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
  int num32frames= 1000;
  sdcardinit();
  /** initializes sd card file for recording.*/
  if (!file.open("testfile1.bin", O_RDWR | O_CREAT)) {
    errorHalt("open failed");
  }
  if (!file.truncate(0)) {
    errorHalt("truncate failed");
  }
  sdfileinit("testfile1.bin");
  setupflexiodma();
  setupflexio(1000);
  int maxtime = 0;
  for(int i=0; i<num32frames; i++){
    while( (framebufferwritepointer-framebufferreadpointer)%framebuffersize < framesize*64);
    //write 32 frames
    //Serial.println(framecount);
    //Serial.println((framebufferwritepointer-framebufferreadpointer)%framebuffersize);
    uint32_t t = micros();
    /** Write to sd card file.*/
    if (framesize*32 != file.write(&framebuffer[framebufferreadpointer], framesize*32)) {
      errorHalt("write failed");
    }
    framebufferreadpointer= (framebufferreadpointer+framesize*32)%framebuffersize;
    t = micros() - t;
    if(t>maxtime) maxtime = t;
  }
  //first turn off flexio, this disables DMA requests
  closeflexio();

  delay(1000);
  Serial.printf("max write time: %d\n",maxtime);
  Serial.println("safe to remove file");
  Serial.printf("framecount: %d\n",framecount);
  Serial.printf("skippedframe: %d\n",skippedframes);
  Serial.println("The next reg should be 0x0 if the DMA kept up with the data");
  PRREG(IMXRT_FLEXIO2_S.SHIFTERR);
  file.rewind();
  for (int frm=0; frm<(num32frames*32); frm++){
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
  while(1);//done
}
