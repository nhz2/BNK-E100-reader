//send out clocks
#include <pindefs.h>
#include <flexiodmaisr.h>
#include <AD7866parsing.h>
#include "SdFat.h"

/******* Globals *********/
bool inrecording= false;
uint32_t framechunkssaved= 0;
uint32_t framechunkstosave= 0;
uint8_t lastsavedframe[framesize]={0};



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
  Serial.setTimeout(10);
  while (!Serial);
  delay(1000);
  rgb.begin();
  dac.begin();
  pinMode(Arangepin,OUTPUT);
  digitalWrite(Arangepin,HIGH);// set ADC range to 0-5v
  pinMode(A6a0pin,OUTPUT);
  digitalWrite(A6a0pin,HIGH);// set ADC6 to channel 2
  if (!sd.begin(SdioConfig(FIFO_SDIO))) {
    errorHalt("begin failed");
  }
  if (!file.open("recording.bin", O_RDWR | O_CREAT)) {
    errorHalt("open failed");
  }
  delay(1000);
}

void loop() {
  // put your main code here, to run repeatedly:
  if(Serial.available() && (!inrecording || numbytesinframebuffer()<framebuffersize/2)){
    //proccess serial commands if available and half empty
    byte cmd= Serial.read();
    switch(cmd)
    {
      case 'a'://resend ack
        Serial.readStringUntil('\n');//clear out rest of command.
        Serial.println('a');
        break;
      case 'd'://set DAC voltage
        float daccmd= Serial.parseFloat();
        dac.setvoltage(daccmd);
        Serial.readStringUntil('\n');//clear out rest of command.      
        Serial.println('a');
        break;
      case 'r'://start recording
        if(inrecording){
          Serial.readStringUntil('\n');//clear out rest of command.      
          Serial.println("in recording");
          break;
        }
        float framerate= Serial.parseFloat();
        framechunkstosave= Serial.parseInt();
        long auxchannel= Serial.parseInt();
        long range= Serial.parseInt();
        userdata0= Serial.parseInt();
        userdata1= Serial.parseInt();
        digitalWrite(Arangepin,range);// set ADC range to 0-5v or 0-2.5v
        digitalWrite(A6a0pin,auxchannel-1);// set ADC6 to channel
        if (!file.truncate(0)) {
          errorHalt("truncate failed");
        }
        setupflexiodma();
        double realrate= setupflexio(framerate);
        Serial.println(realrate);
        Serial.readStringUntil('\n');//clear out rest of command.      
        Serial.println('a');
        inrecording=true;
        rgb.G(true);
        framechunkssaved=0;
        break;
      case 's'://status message
        if(inrecording){
          Serial.print("1,");
          Serial.print(framechunkssaved);
          Serial.print(",");
          Serial.print(skippedframes);
          Serial.print(",");
          Serial.write(lastsavedframe,framesize);
          Serial.println();
          Serial.readStringUntil('\n');//clear out rest of command.      
          Serial.println('a');
          break;
        } else {
          Serial.print("0,");
          Serial.print(framechunkssaved);
          Serial.print(",");
          Serial.print(skippedframes);
          Serial.print(",");
          Serial.write(lastsavedframe,framesize);
          Serial.println();
          Serial.readStringUntil('\n');//clear out rest of command.      
          Serial.println('a');
          break;
        }
      case 'e'://eject card
        closeflexio();
        inrecording=false;
        rgb.G(false);
        if (!file.close()) {
          errorHalt("file close failed");
        }
        Serial.readStringUntil('\n');//clear out rest of command.      
        Serial.println('a');
        break;
      case 'f'://read out frame chunk
        if(inrecording){
          Serial.readStringUntil('\n');//clear out rest of command.      
          Serial.println("in recording");
          break;
        }
        if (!file.sync()) {
          errorHalt("file sync failed");
        }
        int chunkid= Serial.parseInt();
        uint64_t pos= ((uint64_t)chunkid)*framechunksize;
        if (!file.seek(pos)) {
          errorHalt("file seek failed");
        }
        uint8_t chunkdata[framechunksize];
        if (framechunksize != file.read(chunkdata,framechunksize)) {
          errorHalt("file read failed");
        }
        Serial.write(chunkdata,framechunksize);
        Serial.println();
        Serial.readStringUntil('\n');//clear out rest of command.      
        Serial.println('a');
        break;
      default:
        Serial.print(cmd);
        Serial.println(" not valid command");
        Serial.readStringUntil('\n');//clear out rest of command.      
    }
  }

  if(inrecording){
    //check for flexio dma errors
    if(IMXRT_FLEXIO2_S.SHIFTERR!=0) {
      errorHalt("FlexIO2 DMA error");
    }
    if(numbytesinframebuffer() >= framechunksize*2){
      //write a frame chunk
      /** Write to sd card file.*/
      if (framechunksize != file.write((void*)(&framebuffer[framebufferreadpointer]), framechunksize)) {
        errorHalt("write failed");
      }
      //save last saved frame for GUI uses
      memcpy(lastsavedframe,&framebuffer[framebufferreadpointer+framechunksize-framesize],framesize);
      framebufferreadpointer= (framebufferreadpointer+framechunksize)%framebuffersize;
      framechunkssaved++;
    }
    if(framechunkssaved>=framechunkstosave){
      closeflexio();
      inrecording=false;
      rgb.G(false);
    }
  }
}
