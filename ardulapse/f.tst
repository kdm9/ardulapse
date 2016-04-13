#include <SD.h>
#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include "memorysaver.h"


#define SD_CS 4   //SD Card initialisation
const int CS1 = 15;
const int CS2 = 17;

// Instanciation of the class ArduCAM
ArduCAM myCAM1(OV5642, CS1);
ArduCAM myCAM2(OV5642, CS2);
static int k = 0;

// ========= TAKE PICTURE camera 1 ========

void takePicture1(){
  uint8_t start_capture = 0;
  uint8_t temp,temp_last;
  Serial.println("Taking picture cam 1...");
  myCAM1.clear_bit(ARDUCHIP_GPIO, GPIO_POWER_MASK);
  myCAM1.write_reg(ARDUCHIP_MODE, 0x00);
  File outFile;
 
  // Initialisation
  //Change to JPEG capture mode and initialize the OV5642 module 
  myCAM1.set_format(JPEG);
   Serial.print("Init ? ");
  myCAM1.InitCAM();
  Serial.println("OK");
 
  myCAM1.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);    //VSYNC is active HIGH //Change to JPEG capture mode and initialize the OV5642 module 
  myCAM1.write_reg(ARDUCHIP_FRAMES,0x00);
  myCAM1.flush_fifo(); // clean ArduCAM buffer
  myCAM1.OV5642_set_JPEG_size(OV5642_2592x1944);
   
  myCAM1.clear_fifo_flag(); // start capture
  myCAM1.start_capture();
  Serial.print("Waiting for capture cam 1..."); //waiting until capture is done
  while (!(myCAM1.read_reg(ARDUCHIP_TRIG) & CAP_DONE_MASK)) {
   delay(10);
  }
  Serial.println("OK");

 
  // open a file onto the SD card
      outFile = SD.open("Cam1.jpg",O_WRITE | O_CREAT | O_TRUNC); //(O_WRITE | O_CREAT | O_TRUNC) will open the file for write, creating it if it doesn't exist, and truncating it's length to 0 if it does
      if (! outFile){
        Serial.println("open file failed");
      } else {
        Serial.println("File opened sucessfully");
        }

     
    // writes the content of the ArduCAM buffer into the file, with the right
    // function, defined by the 'mode' value
 
    Serial.print("Buffering...");
    while( (temp != 0xD9) | (temp_last != 0xFF) )
     {
      temp_last = temp;
      temp = myCAM1.read_fifo();
      //Write image data to file
      outFile.write(temp);
     }
    Serial.println("OK");
   
    // close the file and clean the flags and the buffers
    outFile.close();
    Serial.println("Capture finished");
    myCAM1.clear_fifo_flag();
    start_capture = 0;
    myCAM1.flush_fifo();
    myCAM1.set_bit(ARDUCHIP_GPIO, GPIO_POWER_MASK);//enable low power
    //myCAM1.InitCAM();
}

// ========= TAKE PICTURE camera 2 ========

void takePicture2(){
  uint8_t start_capture = 0;
  uint8_t temp,temp_last;
  File outFile;
  Serial.println("Taking picture cam 2...");
  myCAM2.clear_bit(ARDUCHIP_GPIO, GPIO_POWER_MASK);
  myCAM2.write_reg(ARDUCHIP_MODE, 0x00);
 
  // Initialisation
  //Change to JPEG capture mode and initialize the OV5642 module 
  myCAM2.set_format(JPEG);
   Serial.print("Init ? ");
  myCAM2.InitCAM();
  Serial.println("OK");
 
  //myCAM2.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);    //VSYNC is active HIGH //Change to JPEG capture mode and initialize the OV5642 module 
  myCAM2.write_reg(ARDUCHIP_FRAMES,0x00);
  myCAM2.flush_fifo(); // clean ArduCAM buffer
  myCAM2.OV5642_set_JPEG_size(OV5642_2592x1944);
  myCAM2.clear_fifo_flag(); // start capture
  myCAM2.start_capture();
  Serial.print("Waiting for capture cam 2..."); //waiting until capture is done
  while (!(myCAM2.read_reg(ARDUCHIP_TRIG) & CAP_DONE_MASK)) {
   delay(10);
  }
  Serial.println("OK");

 
  // open a file onto the SD card
      outFile = SD.open("Cam2.jpg",O_WRITE | O_CREAT | O_TRUNC);
      if (! outFile){
      Serial.println("open file failed");
      } else {
      Serial.println("File opened sucessfully");
      }

     
    // writes the content of the ArduCAM buffer into the file, with the right
    // function, defined by the 'mode' value

    Serial.print("Buffering...");
    while( (temp != 0xD9) | (temp_last != 0xFF) )
     {
      temp_last = temp;
      temp = myCAM2.read_fifo();
      //Write image data to file
      outFile.write(temp);
     }
    Serial.println("OK");
   
    // close the file and clean the flags and the buffers
    outFile.close();
    Serial.println("Capture finished");
    myCAM2.clear_fifo_flag();
    start_capture = 0;
    myCAM2.flush_fifo();
    myCAM2.set_bit(ARDUCHIP_GPIO, GPIO_POWER_MASK);//enable low power
    //myCAM2.InitCAM();
}


// ======== SETUP ========

void setup(){
  uint8_t vid,pid;
  uint8_t temp;
  #if defined (__AVR__)
    Wire.begin();
  #endif
  #if defined(__arm__)
    Wire1.begin();
  #endif
  // begins the Serial communication
  Serial.begin(115200);
  Serial.println("ArduCAM Start!");

  pinMode(CS1, OUTPUT);
  //pinMode(CS2, OUTPUT);
  SPI.begin();
 
  myCAM1.write_reg(ARDUCHIP_TEST1, 0x55);
  temp = myCAM1.read_reg(ARDUCHIP_TEST1);
  if(temp != 0x55)
  {
    Serial.println("SPI interface Error on Camera 1!");
    //while(1);
  }
  else {
  Serial.println("SPI All Good for Camera 1");
  myCAM1.set_bit(ARDUCHIP_GPIO, GPIO_POWER_MASK);//enable low power
  }
 
 myCAM2.write_reg(ARDUCHIP_TEST1, 0x55);
 temp = myCAM2.read_reg(ARDUCHIP_TEST1);
 if(temp != 0x55)
 {
    Serial.println("SPI interface Error on Camera 2!");
    //while(1);
  }
  else {
  Serial.println("SPI All Good for Camera 2");
  myCAM2.set_bit(ARDUCHIP_GPIO, GPIO_POWER_MASK);//enable low power
  }
 
   // SD card initialisation
  if (!SD.begin(SD_CS))
  {
    while (1);    //If failed, stop here
    Serial.println("SD Card Error");
  }
  else
 {   Serial.println("SD Card detected!");
 }

}
// ====== Main Loop ======


void loop(){
    takePicture1();
    takePicture2();
    Serial.println("waiting...");
    delay(12000);
    Serial.println("done...");
 
}
